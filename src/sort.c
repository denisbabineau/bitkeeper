#include "system.h"
#include "sccs.h"

typedef	struct	mem {
	char	*data;		/* pointer to the full block */
	char	*avail;		/* pointer to the first free byte */
	int	left;		/* avail..end */
	struct	mem *next;
} mem_t;

private	int	(*sortfcn)(const void *a, const void *b);
private	int	sortfield = 0;

private mem_t	*
moreMem(mem_t *m)
{
	mem_t	*n = calloc(sizeof(mem_t), 1);

	unless (n && (n->data = malloc(n->left = 4<<20))) return (0);
	n->avail = n->data;
	n->next = m;
	return (n);
}

private int
number_sort(const void *a, const void *b)
{
	int	l, r;

	l = atoi(*(char**)a);
	r = atoi(*(char**)b);
	return (l - r);
}

/*
 * sort on the Nth field.  Split by whitespace only.
 */
private	int
field_sort(const void *a, const void *b)
{
	char	*na, *nb;
	int	i;

	na = *(char **)a;
	na += strspn(na, " \t");
	for (i = 1; i < sortfield; i++) {
		na += strcspn(na, " \t");
		na += strspn(na, " \t");
	}
	nb = *(char **)b;
	nb += strspn(nb, " \t");
	for (i = 1; i < sortfield; i++) {
		nb += strcspn(nb, " \t");
		nb += strspn(nb, " \t");
	}
	return (sortfcn(&na, &nb));
}

/*
 * A simple sort clone.  This is often used to sort keys in the ChangeSet
 * file so the allocations are sized to do that efficiently.
 */
int
sort_main(int ac, char **av)
{
	char	buf[MAXKEY*2];
	char	*p;
	char	**lines = allocLines(50000);
	char	*last = 0;
	int	n = 0;
	int	uflag = 0, rflag = 0, nflag = 0;
	int	i, c;
	mem_t	*mem;

	while ((c = getopt(ac, av, "k:nru")) != -1) {
		switch (c) {
		    case 'k': sortfield = atoi(optarg); break;
		    case 'n': nflag = 1; break;
		    case 'r': rflag = 1; break;
		    case 'u': uflag = 1; break;
		    default:
			system("bk help -s _sort");
		}
	}

	unless (lines && (mem = moreMem(0))) {
		perror("malloc");
		exit(1);
	}
	setmode(0, _O_TEXT);  /* no-op on unix */
	while (fgets(buf, sizeof(buf), stdin)) {
		chop(buf);
		i = strlen(buf) + 1;
		unless (mem->left > i) {
			unless (mem = moreMem(mem)) {
				perror("malloc");
				exit(1);
			}
		}
		p = mem->avail;
		strcpy(p, buf);
		mem->avail += i;
		mem->left -= i;
		lines = addLine(lines, p);
		n++;
	}
	sortfcn = nflag ? number_sort : string_sort;
	qsort((void*)&lines[1], n, sizeof(char *),
	    (sortfield ? field_sort : sortfcn));
	if (rflag) reverseLines(lines);
	EACH(lines) {
		if (uflag && last && streq(last, lines[i])) continue;
		puts(lines[i]);
		last = lines[i];
	}
	free(lines);
	while (mem) {
		mem_t	*memnext = mem->next;
		free(mem->data);
		free(mem);
		mem = memnext;
	}
	return (0);
}