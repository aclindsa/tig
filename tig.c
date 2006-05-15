 * Browse changes in a git repository. Additionally, tig(1) can also act
 * as a pager for output of various git commands.
 *
 * Commit limiting
 * ~~~~~~~~~~~~~~~
 * To speed up interaction with git, you can limit the amount of commits
 * to show both for the log and main view. Either limit by date using
 * e.g. `--since=1.month` or limit by the number of commits using `-n400`.
 *
 * Alternatively, commits can be limited to a specific range, such as
 * "all commits between tag-1.0 and tag-2.0". For example:
 *
 *	$ tig log tag-1.0..tag-2.0
 *
 * Git interprets this as "all commits reachable from commit-2
 * but not from commit-1". The above can also be written:
 *
 *	$ tig log tag-2.0 ^tag-1.0
 *
 * You can think of '^' as a negator. Using this alternate syntax,
 * it is possible to furthur prune commits by specifying multiple
 * negators.
 *
 * This way of commit limiting makes it trivial to only browse the commit
 * which hasn't been pushed to a remote branch. Assuming origin is your
 * upstream remote branch, using:
 *
 *	$ tig log origin..HEAD
 *
 * Optionally, with "HEAD" left out, will list what will be pushed to
 * the remote branch.
 *
 * See the section on environment variables, on how to further tune the
 * interaction with git.
#define TABSIZE		8


	REQ_SCREEN_RESIZE,
	REQ_MOVE_UP_ENTER,
	REQ_MOVE_DOWN_ENTER,
struct ref {
	char *name;
	char id[41];
	unsigned int tag:1;
	unsigned int next:1;
};

	struct ref **refs;	/* Repository references; tags & branch heads. */


/* Option and state variables. */
static bool opt_line_number	= FALSE;
static int opt_tab_size		= TABSIZE;
		/**
		 * -t[NSPACES], --tab-size[=NSPACES]::
		 *	Set the number of spaces tabs should be expanded to.
		 **/
		if (!strncmp(opt, "-t", 2) ||
		    !strncmp(opt, "--tab-size", 10)) {
			char *num = opt;

			if (opt[1] == 't') {
				num = opt + 2;

			} else if (opt[STRING_SIZE("--tab-size")] == '=') {
				num = opt + STRING_SIZE("--tab-size=");
			}

			if (isdigit(*num))
				opt_tab_size = MIN(atoi(num), TABSIZE);
			continue;
		}

 * Below the default key bindings are shown.
	 *	If on a commit line show the commit diff. Addiionally, if in
	 *	main or log view this will split the view. To open the commit
	 *	diff in full size view either use 'd' or press Return twice.
	 *
	 * Up::
	 * Down::
	 * k::
	 *
	 *	Move curser one line up and enter. When used in the main view
	 *	this will always show the diff of the current commit in the
	 *	split diff view.
	 *
	 * j::
	 *	Move cursor one line down and enter.
	 * PgUp::
	 * PgDown::
	{ 'k',		REQ_MOVE_UP_ENTER },
	{ 'j',		REQ_MOVE_DOWN_ENTER },
	 * q::
	 *	Stop all background loading. This can be useful if you ran
	 *	tig(1) in a repository with a long history without limiting
	 *	the log output.
	 *	Open prompt. This allows you to specify what git command
	 *	to run. Example:

	/* Use the ncurses SIGWINCH handler. */
	{ KEY_RESIZE,	REQ_SCREEN_RESIZE },
LINE(DIFF_OLDMODE, "old file mode ",	COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_NEWMODE, "new file mode ",	COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(MAIN_DELIM,   "",			COLOR_MAGENTA,	COLOR_DEFAULT,	0), \
LINE(MAIN_TAG,     "",			COLOR_MAGENTA,	COLOR_DEFAULT,	A_BOLD), \
LINE(MAIN_REF,     "",			COLOR_CYAN,	COLOR_DEFAULT,	A_BOLD),
 * Several options related to the interface with git can be configured
 * via environment options.
 *
 * Repository references
 * ~~~~~~~~~~~~~~~~~~~~~
 * Commits that are referenced by tags and branch heads will be marked
 * by the reference name surrounded by '[' and ']':
 *
 *	2006-04-18 23:12 Jonas Fonseca       | [tig-0.2] tig version 0.2
 *
 * If you want to filter out certain directories under `.git/refs/`, say
 * `tmp` you can do it by setting the following variable:
 *
 *	$ TIG_LS_REMOTE="git ls-remote . | sed /\/tmp\//d" tig
 *
 * Or set the variable permanently in your environment.
 *
 * TIG_LS_REMOTE::
 *	Set command for retrieving all repository references. The command
 *	should output data in the same format as git-ls-remote(1).
 **/

#define TIG_LS_REMOTE \
	"git ls-remote ."

/**
 * View commands
 * ~~~~~~~~~~~~~
 * If for example you prefer commits in the main view to be sorted by date
 * and only show 500 commits, use:
 *	The command used for the log view. If you prefer to have both
 *	author and committer shown in the log view be sure to pass
 *	`--pretty=fuller` to git log.
		/* What type of content being displayed. Used in the
		 * title bar. */
		char *type;
		/* Keep the size of the all view windows one lager than is
		 * required. This makes current line management easier when the
		 * cursor will go outside the window. */
			view->win = newwin(view->height + 1, 0, offset, 0);
			wresize(view->win, view->height + 1, view->width);
			view->ops->type,
		if (view->lineno == view->offset + view->height) {
			/* Clear the hidden line so it doesn't show if the view
			 * is scrolled up. */
			wmove(view->win, view->height, 0);
			wclrtoeol(view->win);
		}
	case REQ_MOVE_UP_ENTER:
	case REQ_MOVE_DOWN_ENTER:
		int linelen = strlen(line);
			report("");
		if (!backgrounded)
			update_view_title(prev);

	/* If the view is backgrounded the above calls to report()
	 * won't redraw the view title. */
	if (backgrounded)
		update_view_title(view);
	case REQ_MOVE_UP_ENTER:
	case REQ_MOVE_DOWN_ENTER:
		move_view(view, request);
		/* Fall-through */

		report("");
		update_view_title(view);
				report("Stopped loaded the %s view", view->name),
	case REQ_SCREEN_RESIZE:
		resize_display();
		/* Fall-through */
	wmove(view->win, lineno, 0);

		wchgat(view->win, -1, 0, type, NULL);
	if (opt_line_number || opt_tab_size < TABSIZE) {
		static char spaces[] = "                    ";
		int col_offset = 0, col = 0;

		if (opt_line_number) {
			unsigned long real_lineno = view->offset + lineno + 1;
			if (real_lineno == 1 ||
			    (real_lineno % opt_num_interval) == 0) {
				wprintw(view->win, "%.*d", view->digits, real_lineno);
			} else {
				waddnstr(view->win, spaces,
					 MIN(view->digits, STRING_SIZE(spaces)));
			}
			waddstr(view->win, ": ");
			col_offset = view->digits + 2;
		}
		while (line && col_offset + col < view->width) {
			int cols_max = view->width - col_offset - col;
			char *text = line;
			int cols;
				assert(sizeof(spaces) > TABSIZE);
				text = spaces;
				cols = opt_tab_size - (col % opt_tab_size);
				line = strchr(line, '\t');
				cols = line ? line - text : strlen(text);

			waddnstr(view->win, text, MIN(cols, cols_max));
			col += cols;
		int col = 0, pos = 0;
		for (; pos < linelen && col < view->width; pos++, col++)
			if (line[pos] == '\t')
				col += TABSIZE - (col % TABSIZE) - 1;

		waddnstr(view->win, line, pos);
	}
	/* Compress empty lines in the help view. */
		if (view == VIEW(REQ_VIEW_LOG))
			open_view(view, REQ_VIEW_DIFF, OPEN_SPLIT | OPEN_BACKGROUNDED);
		else
			open_view(view, REQ_VIEW_DIFF, OPEN_DEFAULT);
	"line",
static struct ref **get_refs(char *id);

	int col = 0;
	wmove(view->win, lineno, col);

		wattrset(view->win, get_line_attr(type));
		wchgat(view->win, -1, 0, type, NULL);

		wattrset(view->win, get_line_attr(LINE_MAIN_DATE));
	col += DATE_COLS;
	wmove(view->win, lineno, col);
	if (type != LINE_CURSOR)
		wattrset(view->win, get_line_attr(LINE_MAIN_AUTHOR));
		if (type != LINE_CURSOR)
			wattrset(view->win, get_line_attr(LINE_MAIN_DELIM));
	col += 20;
	if (type != LINE_CURSOR)
		wattrset(view->win, A_NORMAL);

	mvwaddch(view->win, lineno, col, ACS_LTEE);
	wmove(view->win, lineno, col + 2);
	col += 2;

	if (commit->refs) {
		size_t i = 0;

		do {
			if (type == LINE_CURSOR)
				;
			else if (commit->refs[i]->tag)
				wattrset(view->win, get_line_attr(LINE_MAIN_TAG));
			else
				wattrset(view->win, get_line_attr(LINE_MAIN_REF));
			waddstr(view->win, "[");
			waddstr(view->win, commit->refs[i]->name);
			waddstr(view->win, "]");
			if (type != LINE_CURSOR)
				wattrset(view->win, A_NORMAL);
			waddstr(view->win, " ");
			col += strlen(commit->refs[i]->name) + STRING_SIZE("[] ");
		} while (commit->refs[i++]->next);
	}

	if (type != LINE_CURSOR)
		wattrset(view->win, get_line_attr(type));

	{
		int titlelen = strlen(commit->title);

		if (col + titlelen > view->width)
			titlelen = view->width - col;

		waddnstr(view->win, commit->title, titlelen);
	}
		commit->refs = get_refs(commit->id);
	"commit",

	static bool empty = TRUE;
	struct view *view = display[current_view];

	if (!empty || *msg) {
		va_list args;
		va_start(args, msg);
		werase(status_win);
		wmove(status_win, 0, 0);
		if (*msg) {
			vwprintw(status_win, msg, args);
			empty = FALSE;
		} else {
			empty = TRUE;
		}
		wrefresh(status_win);
		va_end(args);
	}
	update_view_title(view);

	/* Move the cursor to the right-most column of the cursor line.
	 *
	 * XXX: This could turn out to be a bit expensive, but it ensures that
	 * the cursor does not jump around. */
	if (view->lines) {
		wmove(view->win, view->lineno - view->offset, view->width - 1);
		wrefresh(view->win);
	}
	static unsigned int loading_views;
	if ((loading == FALSE && loading_views-- == 1) ||
	    (loading == TRUE  && loading_views++ == 0))

/*
 * Repository references
 */

static struct ref *refs;
size_t refs_size;

static struct ref **
get_refs(char *id)
{
	struct ref **id_refs = NULL;
	size_t id_refs_size = 0;
	size_t i;

	for (i = 0; i < refs_size; i++) {
		struct ref **tmp;

		if (strcmp(id, refs[i].id))
			continue;

		tmp = realloc(id_refs, (id_refs_size + 1) * sizeof(*id_refs));
		if (!tmp) {
			if (id_refs)
				free(id_refs);
			return NULL;
		}

		id_refs = tmp;
		if (id_refs_size > 0)
			id_refs[id_refs_size - 1]->next = 1;
		id_refs[id_refs_size] = &refs[i];

		/* XXX: The properties of the commit chains ensures that we can
		 * safely modify the shared ref. The repo references will
		 * always be similar for the same id. */
		id_refs[id_refs_size]->next = 0;
		id_refs_size++;
	}

	return id_refs;
}

static int
load_refs(void)
{
	char *cmd_env = getenv("TIG_LS_REMOTE");
	char *cmd = cmd_env && *cmd_env ? cmd_env : TIG_LS_REMOTE;
	FILE *pipe = popen(cmd, "r");
	char buffer[BUFSIZ];
	char *line;

	if (!pipe)
		return ERR;

	while ((line = fgets(buffer, sizeof(buffer), pipe))) {
		char *name = strchr(line, '\t');
		struct ref *ref;
		int namelen;
		bool tag = FALSE;
		bool tag_commit = FALSE;

		if (!name)
			continue;

		*name++ = 0;
		namelen = strlen(name) - 1;

		/* Commits referenced by tags has "^{}" appended. */
		if (name[namelen - 1] == '}') {
			while (namelen > 0 && name[namelen] != '^')
				namelen--;
			if (namelen > 0)
				tag_commit = TRUE;
		}
		name[namelen] = 0;

		if (!strncmp(name, "refs/tags/", STRING_SIZE("refs/tags/"))) {
			if (!tag_commit)
				continue;
			name += STRING_SIZE("refs/tags/");
			tag = TRUE;

		} else if (!strncmp(name, "refs/heads/", STRING_SIZE("refs/heads/"))) {
			name += STRING_SIZE("refs/heads/");

		} else if (!strcmp(name, "HEAD")) {
			continue;
		}

		refs = realloc(refs, sizeof(*refs) * (refs_size + 1));
		if (!refs)
			return ERR;

		ref = &refs[refs_size++];
		ref->tag = tag;
		ref->name = strdup(name);
		if (!ref->name)
			return ERR;

		string_copy(ref->id, line);
	}

	if (ferror(pipe))
		return ERR;

	pclose(pipe);

	return OK;
}

	if (load_refs() == ERR)
		die("Failed to load refs.");

		/* Some low-level request handling. This keeps access to
		 * status_win restricted. */
		switch (request) {
		case REQ_PROMPT:
			break;

		case REQ_SCREEN_RESIZE:
		{
			int height, width;

			getmaxyx(stdscr, height, width);

			/* Resize the status view and let the view driver take
			 * care of resizing the displayed views. */
			wresize(status_win, 1, width);
			mvwin(status_win, height - 1, 0);
			wrefresh(status_win);
			break;
		}
		default:
			break;
 * BUGS
 * ----
 * Known bugs and problems:
 *
 * - If the screen width is very small the main view can draw
 *   outside the current view causing bad wrapping.
 *
 * - Searching.