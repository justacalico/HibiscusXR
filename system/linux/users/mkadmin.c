// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hibiscus-mkadmin - mint a sudo-capable account.
 *
 *         hibiscus-mkadmin <name>
 *
 *         The stock hibiscus login has no password and no sudo, so
 *         this setuid helper is how a box gets its first admin. It
 *         runs for root, for existing members of the admin group, and
 *         for anyone while the admin group is still empty - that last
 *         case is the bootstrap a fresh image relies on. The account
 *         lands in wheel, or sudo on bases without wheel, and passwd
 *         runs right after so the password is real, not empty.
 *
 *         HIBISCUS_MKADMIN_ROOT=<dir> edits that tree's etc/ instead
 *         of / and skips the passwd hand-off - image assembly and the
 *         test suite run it that way.
 */
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static const char *ROOT = "";

static void
epath(char *buf, size_t n, const char *file)
{
	snprintf(buf, n, "%s/etc/%s", ROOT, file);
}

/* NUL-terminate field `col` of `line` in place, return it or NULL */
static char *
field(char *line, int col)
{
	char *p = line;
	for (int i = 0; i < col && p; i++)
		p = strchr(p, ':') ? strchr(p, ':') + 1 : NULL;
	if (!p)
		return NULL;
	char *e = strpbrk(p, ":\n");
	if (e)
		*e = 0;
	return p;
}

static int
field_taken(const char *file, int col, const char *want)
{
	char path[512], line[1024], work[1024];
	int hit = 0;
	epath(path, sizeof(path), file);
	FILE *f = fopen(path, "r");
	if (!f)
		return 0;
	while (!hit && fgets(line, sizeof(line), f)) {
		snprintf(work, sizeof(work), "%s", line);
		char *v = field(work, col);
		hit = v && !strcmp(v, want);
	}
	fclose(f);
	return hit;
}

static int
id_taken(const char *file, long id)
{
	char want[24];
	snprintf(want, sizeof(want), "%ld", id);
	return field_taken(file, 2, want);
}

static int
name_ok(const char *s)
{
	size_t n = strlen(s);
	if (!n || n > 32)
		return 0;
	if (!islower((unsigned char)s[0]) && s[0] != '_')
		return 0;
	for (size_t i = 1; i < n; i++)
		if (!islower((unsigned char)s[i]) &&
		    !isdigit((unsigned char)s[i]) &&
		    s[i] != '_' && s[i] != '-')
			return 0;
	return 1;
}

/* member list of the first admin group the base offers: a malloc'd
 * string ("" when the group exists but is empty), NULL when the base
 * has neither wheel nor sudo */
static char *
admin_members(char *gname, size_t gnlen)
{
	const char *cand[] = { "wheel", "sudo" };
	char path[512], line[2048], work[2048];
	char *out = NULL;
	epath(path, sizeof(path), "group");
	FILE *f = fopen(path, "r");
	if (!f)
		return NULL;
	for (size_t i = 0; i < sizeof(cand) / sizeof(*cand) && !out; i++) {
		rewind(f);
		while (fgets(line, sizeof(line), f)) {
			snprintf(work, sizeof(work), "%s", line);
			char *n = field(work, 0);
			if (!n || strcmp(n, cand[i]))
				continue;
			snprintf(work, sizeof(work), "%s", line);
			char *m = field(work, 3);
			snprintf(gname, gnlen, "%s", cand[i]);
			out = strdup(m ? m : "");
			break;
		}
	}
	fclose(f);
	return out;
}

static int
listed(const char *members, const char *name)
{
	if (!members || !*members)
		return 0;
	char *copy = strdup(members);
	int hit = 0;
	char *save = NULL;
	for (char *t = strtok_r(copy, ",", &save); t && !hit;
	     t = strtok_r(NULL, ",", &save))
		hit = !strcmp(t, name);
	free(copy);
	return hit;
}

static void
append(const char *file, const char *line)
{
	char path[512];
	epath(path, sizeof(path), file);
	FILE *f = fopen(path, "a");
	if (!f) {
		fprintf(stderr, "mkadmin: %s: %s\n", path, strerror(errno));
		exit(1);
	}
	fputs(line, f);
	fclose(f);
}

static int
exists(const char *file)
{
	char path[512];
	epath(path, sizeof(path), file);
	return access(path, F_OK) == 0;
}

/* rewrite /etc/group adding `name` to `gname`'s member list */
static int
group_add(const char *gname, const char *name)
{
	char path[512], tmp[520], line[2048], work[2048];
	char *txt = NULL;
	size_t cap = 0;
	int done = 0;
	epath(path, sizeof(path), "group");
	FILE *f = fopen(path, "r");
	if (!f)
		return -1;
	while (fgets(line, sizeof(line), f)) {
		snprintf(work, sizeof(work), "%s", line);
		char *n = field(work, 0);
		if (!done && n && !strcmp(n, gname)) {
			size_t l = strlen(line);
			if (l && line[l - 1] == '\n')
				line[--l] = 0;
			snprintf(line + l, sizeof(line) - l, "%s%s\n",
				 l && line[l - 1] == ':' ? "" : ",", name);
			done = 1;
		}
		size_t l = strlen(line);
		txt = realloc(txt, cap + l + 1);
		memcpy(txt + cap, line, l + 1);
		cap += l;
	}
	fclose(f);
	if (!done || !txt) {
		free(txt);
		return -1;
	}
	snprintf(tmp, sizeof(tmp), "%s.mkadmin", path);
	f = fopen(tmp, "w");
	if (!f) {
		free(txt);
		return -1;
	}
	fwrite(txt, 1, cap, f);
	fclose(f);
	chmod(tmp, 0644);
	rename(tmp, path);
	free(txt);
	return 0;
}

static void
make_home(const char *name, long uid, long gid)
{
	char dir[600];
	snprintf(dir, sizeof(dir), "%s/home", ROOT);
	mkdir(dir, 0755);
	snprintf(dir, sizeof(dir), "%s/home/%s", ROOT, name);
	if (mkdir(dir, 0700) < 0 && errno != EEXIST)
		fprintf(stderr, "mkadmin: %s: %s\n", dir, strerror(errno));
	chown(dir, uid, gid);
}

int
main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: hibiscus-mkadmin <name>\n");
		return 2;
	}
	const char *name = argv[1];
	const char *r = getenv("HIBISCUS_MKADMIN_ROOT");
	if (r && *r)
		ROOT = r;
	if (!name_ok(name)) {
		fprintf(stderr, "mkadmin: bad user name %s\n", name);
		return 2;
	}
	if (field_taken("passwd", 0, name) ||
	    field_taken("group", 0, name)) {
		fprintf(stderr, "mkadmin: %s already exists\n", name);
		return 1;
	}
	char grp[64];
	char *members = admin_members(grp, sizeof(grp));
	if (!members) {
		fprintf(stderr,
			"mkadmin: no wheel or sudo group - nothing grants sudo\n");
		return 1;
	}
	if (getuid() != 0) {
		struct passwd *pw = getpwuid(getuid());
		const char *me = pw ? pw->pw_name : "";
		if (!listed(members, me) && *members) {
			fprintf(stderr,
				"mkadmin: %s already has members - ask one of them\n",
				grp);
			free(members);
			return 1;
		}
	}
	free(members);
	if (!r)
		lckpwdf();

	long uid = 1000;
	while (id_taken("passwd", uid))
		uid++;
	long gid = uid;
	while (id_taken("group", gid))
		gid++;
	char bash[600];
	snprintf(bash, sizeof(bash), "%s/bin/bash", ROOT);
	const char *shell = access(bash, X_OK) == 0 ? "/bin/bash" : "/bin/sh";

	char buf[1024];
	snprintf(buf, sizeof(buf), "%s:x:%ld:\n", name, gid);
	append("group", buf);
	if (exists("gshadow")) {
		snprintf(buf, sizeof(buf), "%s:!::\n", name);
		append("gshadow", buf);
	}
	if (exists("shadow")) {
		snprintf(buf, sizeof(buf),
			 "%s:x:%ld:%ld:Hibiscus admin:/home/%s:%s\n",
			 name, uid, gid, name, shell);
		append("passwd", buf);
		snprintf(buf, sizeof(buf), "%s::%ld:0:99999:7:::\n",
			 name, (long)(time(NULL) / 86400));
		append("shadow", buf);
	} else {
		snprintf(buf, sizeof(buf),
			 "%s::%ld:%ld:Hibiscus admin:/home/%s:%s\n",
			 name, uid, gid, name, shell);
		append("passwd", buf);
	}
	make_home(name, uid, gid);
	if (group_add(grp, name) < 0)
		fprintf(stderr, "mkadmin: could not add %s to %s\n", name, grp);
	if (!r)
		ulckpwdf();

	printf("created %s in %s\n", name, grp);
	if (r)
		return 0;
	setuid(0);
	execl("/usr/bin/passwd", "passwd", name, (char *)NULL);
	execl("/bin/passwd", "passwd", name, (char *)NULL);
	fprintf(stderr, "mkadmin: set a password later with passwd %s\n",
		name);
	return 0;
}
