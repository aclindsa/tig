/* Copyright (c) 2006-2014 Jonas Fonseca <jonas.fonseca@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "tig/repo.h"
#include "tig/options.h"
#include "tig/parse.h"
#include "tig/watch.h"
#include "tig/graph.h"
#include "tig/display.h"
#include "tig/view.h"
#include "tig/draw.h"
#include "tig/git.h"
#include "tig/status.h"
#include "tig/stage.h"
#include "tig/main.h"
#include "tig/diff.h"

/*
 * Main view backend
 */

DEFINE_ALLOCATOR(realloc_reflogs, char *, 32)

static void
main_register_commit(struct view *view, struct commit *commit, const char *ids, bool is_boundary)
{
	struct main_state *state = view->private;

	string_copy_rev(commit->id, ids);
	if (state->with_graph)
		graph_add_commit(&state->graph, &commit->graph, commit->id, ids, is_boundary);
}

static struct commit *
main_add_commit(struct view *view, enum line_type type, struct commit *template,
		const char *title, bool custom)
{
	struct main_state *state = view->private;
	size_t titlelen;
	struct commit *commit;
	char buf[SIZEOF_STR / 2];
	struct line *line;

	/* FIXME: More graceful handling of titles; append "..." to
	 * shortened titles, etc. */
	string_expand(buf, sizeof(buf), title, 1);
	title = buf;
	titlelen = strlen(title);

	line = add_line_alloc(view, &commit, type, titlelen, custom);
	if (!line)
		return NULL;

	*commit = *template;
	strncpy(commit->title, title, titlelen);
	state->graph.canvas = &commit->graph;
	memset(template, 0, sizeof(*template));
	state->reflogmsg[0] = 0;

	view_column_info_update(view, line);
	return commit;
}

static inline void
main_flush_commit(struct view *view, struct commit *commit)
{
	if (*commit->id)
		main_add_commit(view, LINE_MAIN_COMMIT, commit, "", FALSE);
}

static bool
main_add_changes_commit(struct view *view, enum line_type type, const char *parent, const char *title)
{
	char ids[SIZEOF_STR] = NULL_ID " ";
	struct main_state *state = view->private;
	struct commit commit = {};
	struct timeval now;
	struct timezone tz;

	if (!parent)
		return TRUE;

	if (*parent)
		string_copy_rev(ids + STRING_SIZE(NULL_ID " "), parent);
	else
		ids[STRING_SIZE(NULL_ID)] = 0;

	if (!gettimeofday(&now, &tz)) {
		commit.time.tz = tz.tz_minuteswest * 60;
		commit.time.sec = now.tv_sec - commit.time.tz;
	}

	commit.author = &unknown_ident;
	main_register_commit(view, &commit, ids, FALSE);
	if (!main_add_commit(view, type, &commit, title, TRUE))
		return FALSE;

	if (state->with_graph) {
		if (*parent)
			return graph_render_parents(&state->graph);
		state->add_changes_parents = TRUE;
	}

	return TRUE;
}

static bool
main_add_changes_commits(struct view *view, struct main_state *state, const char *parent)
{
	const char *staged_parent = NULL_ID;
	const char *unstaged_parent = parent;
	struct index_diff diff;

	if (!index_diff(&diff, FALSE, FALSE))
		return FALSE;

	if (!diff.unstaged) {
		unstaged_parent = NULL;
		staged_parent = parent;
		watch_apply(&view->watch, WATCH_INDEX_UNSTAGED_NO);
	} else {
		watch_apply(&view->watch, WATCH_INDEX_UNSTAGED_YES);
	}

	if (!diff.staged) {
		staged_parent = NULL;
		watch_apply(&view->watch, WATCH_INDEX_STAGED_NO);
	} else {
		watch_apply(&view->watch, WATCH_INDEX_STAGED_YES);
	}

	return main_add_changes_commit(view, LINE_STAT_STAGED, staged_parent, "Staged changes")
	    && main_add_changes_commit(view, LINE_STAT_UNSTAGED, unstaged_parent, "Unstaged changes");
}

static bool
main_check_argv(struct view *view, const char *argv[])
{
	struct main_state *state = view->private;
	bool with_reflog = FALSE;
	int i;

	for (i = 0; argv[i]; i++) {
		const char *arg = argv[i];
		struct rev_flags rev_flags = {};

		if (!strcmp(arg, "--graph")) {
			struct view_column *column = get_view_column(view, VIEW_COLUMN_COMMIT_TITLE);

			if (column) {
				column->opt.commit_title.graph = TRUE;
				if (opt_commit_order != COMMIT_ORDER_REVERSE)
					state->with_graph = TRUE;
			}
			argv[i] = "";
			continue;
		}

		if (!argv_parse_rev_flag(arg, &rev_flags))
			continue;

		if (rev_flags.with_reflog)
			with_reflog = TRUE;
		if (!rev_flags.with_graph)
			state->with_graph = FALSE;
		arg += rev_flags.search_offset;
		if (*arg && !*view->env->search)
			string_ncopy(view->env->search, arg, strlen(arg));
	}

	return with_reflog;
}

static bool
main_with_graph(struct view *view, enum open_flags flags)
{
	struct view_column *column = get_view_column(view, VIEW_COLUMN_COMMIT_TITLE);

	if (open_in_pager_mode(flags))
		return FALSE;

	return column && column->opt.commit_title.graph &&
	       opt_commit_order != COMMIT_ORDER_REVERSE;
}

static bool
main_open(struct view *view, enum open_flags flags)
{
	bool with_graph = main_with_graph(view, flags);
	const char *pretty_custom_argv[] = {
		GIT_MAIN_LOG_CUSTOM(encoding_arg, commit_order_arg_with_graph(with_graph),
			"%(cmdlineargs)", "%(revargs)", "%(fileargs)")
	};
	const char *pretty_raw_argv[] = {
		GIT_MAIN_LOG_RAW(encoding_arg, commit_order_arg_with_graph(with_graph),
			"%(cmdlineargs)", "%(revargs)", "%(fileargs)")
	};
	struct main_state *state = view->private;
	const char **main_argv = pretty_custom_argv;
	enum watch_trigger changes_triggers = WATCH_NONE;

	if (opt_show_changes && repo.is_inside_work_tree)
		changes_triggers |= WATCH_INDEX;

	state->with_graph = with_graph;

	if (opt_rev_args && main_check_argv(view, opt_rev_args))
		main_argv = pretty_raw_argv;

	if (open_in_pager_mode(flags)) {
		changes_triggers = WATCH_NONE;
	}

	/* This calls reset_view() so must be before adding changes commits. */
	if (!begin_update(view, NULL, main_argv, flags))
		return FALSE;

	/* Register watch before changes commits are added to record the
	 * start. */
	if (view_can_refresh(view))
		watch_register(&view->watch, WATCH_HEAD | WATCH_REFS | changes_triggers);

	if (changes_triggers)
		main_add_changes_commits(view, state, "");

	return TRUE;
}

void
main_done(struct view *view)
{
	struct main_state *state = view->private;
	int i;

	for (i = 0; i < view->lines; i++) {
		struct commit *commit = view->line[i].data;

		free(commit->graph.symbols);
	}

	for (i = 0; i < state->reflogs; i++)
		free(state->reflog[i]);
	free(state->reflog);
}

#define main_check_commit_refs(line)	!((line)->no_commit_refs)
#define main_mark_no_commit_refs(line)	(((struct line *) (line))->no_commit_refs = 1)

static inline struct ref_list *
main_get_commit_refs(const struct line *line, struct commit *commit)
{
	struct ref_list *refs = NULL;

	if (main_check_commit_refs(line) && !(refs = get_ref_list(commit->id)))
		main_mark_no_commit_refs(line);

	return refs;
}

bool
main_get_column_data(struct view *view, const struct line *line, struct view_column_data *column_data)
{
	struct main_state *state = view->private;
	struct commit *commit = line->data;
	struct ref_list *refs = NULL;

	column_data->author = commit->author;
	column_data->date = &commit->time;
	column_data->id = commit->id;
	if (state->reflogs)
		column_data->reflog = state->reflog[line->lineno - 1];

	column_data->commit_title = commit->title;
	if (state->with_graph)
		column_data->graph = &commit->graph;

	if ((refs = main_get_commit_refs(line, commit)))
		column_data->refs = refs;

	return TRUE;
}

static bool
main_add_reflog(struct view *view, struct main_state *state, char *reflog)
{
	char *end = strchr(reflog, ' ');
	int id_width;

	if (!end)
		return FALSE;
	*end = 0;

	if (!realloc_reflogs(&state->reflog, state->reflogs, 1)
	    || !(reflog = strdup(reflog)))
		return FALSE;

	state->reflog[state->reflogs++] = reflog;
	id_width = strlen(reflog);
	if (state->reflog_width < id_width) {
		struct view_column *column = get_view_column(view, VIEW_COLUMN_ID);

		state->reflog_width = id_width;
		if (column && column->opt.id.display)
			view->force_redraw = TRUE;
	}

	return TRUE;
}

/* Reads git log --pretty=raw output and parses it into the commit struct. */
bool
main_read(struct view *view, struct buffer *buf)
{
	struct main_state *state = view->private;
	struct graph *graph = &state->graph;
	enum line_type type;
	struct commit *commit = &state->current;
	char *line;

	if (!buf) {
		main_flush_commit(view, commit);

		if (failed_to_load_initial_view(view))
			die("No revisions match the given arguments.");
		if (view->lines > 0) {
			struct commit *last = view->line[view->lines - 1].data;

			view->line[view->lines - 1].dirty = 1;
			if (!last->author) {
				view->lines--;
				free(last);
			}
		}

		if (state->with_graph)
			done_graph(graph);
		return TRUE;
	}

	line = buf->data;
	type = get_line_type(line);
	if (type == LINE_COMMIT) {
		bool is_boundary;
		char *author;

		state->in_header = TRUE;
		line += STRING_SIZE("commit ");
		is_boundary = *line == '-';
		while (*line && !isalnum(*line))
			line++;

		if (state->add_changes_parents) {
			state->add_changes_parents = FALSE;
			if (!graph_add_parent(graph, line))
				return FALSE;
			graph->has_parents = TRUE;
			graph_render_parents(graph);
		}

		main_flush_commit(view, commit);
		main_register_commit(view, &state->current, line, is_boundary);

		author = io_memchr(buf, line, 0);
		if (author) {
			char *title = io_memchr(buf, author, 0);

			parse_author_line(author, &commit->author, &commit->time);
			if (state->with_graph)
				graph_render_parents(graph);
			if (title)
				main_add_commit(view, LINE_MAIN_COMMIT, commit, title, FALSE);
		}

		return TRUE;
	}

	if (!*commit->id)
		return TRUE;

	/* Empty line separates the commit header from the log itself. */
	if (*line == '\0')
		state->in_header = FALSE;

	switch (type) {
	case LINE_PP_REFLOG:
		if (!main_add_reflog(view, state, line + STRING_SIZE("Reflog: ")))
			return FALSE;
		break;

	case LINE_PP_REFLOGMSG:
		line += STRING_SIZE("Reflog message: ");
		string_ncopy(state->reflogmsg, line, strlen(line));
		break;

	case LINE_PARENT:
		if (state->with_graph && !graph->has_parents)
			graph_add_parent(graph, line + STRING_SIZE("parent "));
		break;

	case LINE_AUTHOR:
		parse_author_line(line + STRING_SIZE("author "),
				  &commit->author, &commit->time);
		if (state->with_graph)
			graph_render_parents(graph);
		break;

	default:
		/* Fill in the commit title if it has not already been set. */
		if (*commit->title)
			break;

		/* Skip lines in the commit header. */
		if (state->in_header)
			break;

		/* Require titles to start with a non-space character at the
		 * offset used by git log. */
		if (strncmp(line, "    ", 4))
			break;
		line += 4;
		/* Well, if the title starts with a whitespace character,
		 * try to be forgiving.  Otherwise we end up with no title. */
		while (isspace(*line))
			line++;
		if (*line == '\0')
			break;
		if (*state->reflogmsg)
			line = state->reflogmsg;
		main_add_commit(view, LINE_MAIN_COMMIT, commit, line, FALSE);
	}

	return TRUE;
}

enum request
main_request(struct view *view, enum request request, struct line *line)
{
	enum open_flags flags = (view_is_displayed(view) && request != REQ_VIEW_DIFF)
				? OPEN_SPLIT : OPEN_DEFAULT;

	switch (request) {
	case REQ_NEXT:
	case REQ_PREVIOUS:
		if (view_is_displayed(view) && display[0] != view)
			return request;
		/* Do not pass navigation requests to the branch view
		 * when the main view is maximized. (GH #38) */
		return request == REQ_NEXT ? REQ_MOVE_DOWN : REQ_MOVE_UP;

	case REQ_VIEW_DIFF:
	case REQ_ENTER:
		if (view_is_displayed(view) && display[0] != view)
			maximize_view(view, TRUE);

		if (line->type == LINE_STAT_UNSTAGED
		    || line->type == LINE_STAT_STAGED)
			open_stage_view(view, NULL, line->type, flags); 
		else
			open_diff_view(view, flags);
		break;

	case REQ_REFRESH:
		load_refs(TRUE);
		refresh_view(view);
		break;

	default:
		return request;
	}

	return REQ_NONE;
}

static void
main_update_env(struct view *view, struct line *line, struct commit *commit)
{
	struct ref_list *list = main_get_commit_refs(line, commit);
	size_t i;

	for (i = 0; list && i < list->size; i++)
		ref_update_env(view->env, list->refs[list->size - i - 1], !i);
}

void
main_select(struct view *view, struct line *line)
{
	struct commit *commit = line->data;

	if (line->type == LINE_STAT_STAGED || line->type == LINE_STAT_UNSTAGED) {
		string_ncopy(view->ref, commit->title, strlen(commit->title));
		status_stage_info(view->env->status, line->type, NULL);
	} else {
		string_copy_rev(view->ref, commit->id);
		main_update_env(view, line, commit);
	}
	string_copy_rev(view->env->commit, commit->id);
}

static struct view_ops main_ops = {
	"commit",
	argv_env.head,
	VIEW_SEND_CHILD_ENTER | VIEW_FILE_FILTER | VIEW_LOG_LIKE | VIEW_REFRESH,
	sizeof(struct main_state),
	main_open,
	main_read,
	view_column_draw,
	main_request,
	view_column_grep,
	main_select,
	main_done,
	view_column_bit(AUTHOR) | view_column_bit(COMMIT_TITLE) |
		view_column_bit(DATE) |	view_column_bit(ID) |
		view_column_bit(LINE_NUMBER),
	main_get_column_data,
};

DEFINE_VIEW(main);

/* vim: set ts=8 sw=8 noexpandtab: */
