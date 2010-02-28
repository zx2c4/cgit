#include "cgit.h"
#include "configfile.h"
#include "html.h"

#define MAX_PATH 4096

/* return 1 if path contains a objects/ directory and a HEAD file */
static int is_git_dir(const char *path)
{
	struct stat st;
	static char buf[MAX_PATH];

	if (snprintf(buf, MAX_PATH, "%s/objects", path) >= MAX_PATH) {
		fprintf(stderr, "Insanely long path: %s\n", path);
		return 0;
	}
	if (stat(buf, &st)) {
		if (errno != ENOENT)
			fprintf(stderr, "Error checking path %s: %s (%d)\n",
				path, strerror(errno), errno);
		return 0;
	}
	if (!S_ISDIR(st.st_mode))
		return 0;

	sprintf(buf, "%s/HEAD", path);
	if (stat(buf, &st)) {
		if (errno != ENOENT)
			fprintf(stderr, "Error checking path %s: %s (%d)\n",
				path, strerror(errno), errno);
		return 0;
	}
	if (!S_ISREG(st.st_mode))
		return 0;

	return 1;
}

struct cgit_repo *repo;
repo_config_fn config_fn;

static void repo_config(const char *name, const char *value)
{
	config_fn(repo, name, value);
}

static void add_repo(const char *base, const char *path, repo_config_fn fn)
{
	struct stat st;
	struct passwd *pwd;
	char *p;
	size_t size;

	if (stat(path, &st)) {
		fprintf(stderr, "Error accessing %s: %s (%d)\n",
			path, strerror(errno), errno);
		return;
	}
	if (!stat(fmt("%s/noweb", path), &st))
		return;
	if ((pwd = getpwuid(st.st_uid)) == NULL) {
		fprintf(stderr, "Error reading owner-info for %s: %s (%d)\n",
			path, strerror(errno), errno);
		return;
	}
	if (base == path)
		p = fmt("%s", path);
	else
		p = fmt("%s", path + strlen(base) + 1);

	if (!strcmp(p + strlen(p) - 5, "/.git"))
		p[strlen(p) - 5] = '\0';

	repo = cgit_add_repo(xstrdup(p));
	repo->name = repo->url;
	repo->path = xstrdup(path);
	p = (pwd && pwd->pw_gecos) ? strchr(pwd->pw_gecos, ',') : NULL;
	if (p)
		*p = '\0';
	repo->owner = (pwd ? xstrdup(pwd->pw_gecos ? pwd->pw_gecos : pwd->pw_name) : "");

	p = fmt("%s/description", path);
	if (!stat(p, &st))
		readfile(p, &repo->desc, &size);

	p = fmt("%s/README.html", path);
	if (!stat(p, &st))
		repo->readme = "README.html";

	p = fmt("%s/cgitrc", path);
	if (!stat(p, &st)) {
		config_fn = fn;
		parse_configfile(xstrdup(p), &repo_config);
	}
}

static void scan_path(const char *base, const char *path, repo_config_fn fn)
{
	DIR *dir;
	struct dirent *ent;
	char *buf;
	struct stat st;

	if (is_git_dir(path)) {
		add_repo(base, path, fn);
		return;
	}
	if (is_git_dir(fmt("%s/.git", path))) {
		add_repo(base, fmt("%s/.git", path), fn);
		return;
	}
	dir = opendir(path);
	if (!dir) {
		fprintf(stderr, "Error opening directory %s: %s (%d)\n",
			path, strerror(errno), errno);
		return;
	}
	while((ent = readdir(dir)) != NULL) {
		if (ent->d_name[0] == '.') {
			if (ent->d_name[1] == '\0')
				continue;
			if (ent->d_name[1] == '.' && ent->d_name[2] == '\0')
				continue;
		}
		buf = malloc(strlen(path) + strlen(ent->d_name) + 2);
		if (!buf) {
			fprintf(stderr, "Alloc error on %s: %s (%d)\n",
				path, strerror(errno), errno);
			exit(1);
		}
		sprintf(buf, "%s/%s", path, ent->d_name);
		if (stat(buf, &st)) {
			fprintf(stderr, "Error checking path %s: %s (%d)\n",
				buf, strerror(errno), errno);
			free(buf);
			continue;
		}
		if (S_ISDIR(st.st_mode))
			scan_path(base, buf, fn);
		free(buf);
	}
	closedir(dir);
}

void scan_tree(const char *path, repo_config_fn fn)
{
	scan_path(path, path, fn);
}
