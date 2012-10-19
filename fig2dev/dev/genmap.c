/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1999 by T. Sato
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such 
 * party to do so, with the only requirement being that this copyright 
 * notice remain intact.
 *
 */

/*
 * genmap.c: HTML imagemap driver for fig2dev
 *
 * T.Sato <VEF00200@nifty.ne.jp>
 *
 *
 * Description:
 *   Generate HTML imagemap from Fig file.
 *   If an object has comment like ``HREF="url" ALT="string"'',
 *   the object will becomes link to the URL.
 *   Object with smaller depth will be listed before deeper ones.
 *   Figure comment will be used as the default link.
 */

#include "fig2dev.h"
#include "object.h"

#define TEXT_LENGTH  300

static int      border_margin = 0;
static char     url[TEXT_LENGTH], alt[TEXT_LENGTH];
static char     buf[2000];

#define MAX_LINKS 100

static int      n_links = 0;

static struct {
  char *url;
  char *alt;
  char *area;
} links[MAX_LINKS];

#define fscale 15

#define XZOOM(x)   round(mag * ((x) - llx) / fscale + border_margin)
#define YZOOM(y)   round(mag * ((y) - lly) / fscale + border_margin)

#define MIN_DIST    5  /* ignore closer points when processing polyline */

#define ARC_STEP    (M_PI / 9.0)
#define ARC_EXPAND  1.02   /* make polygon slightly larger */


void
genmap_option(opt, optarg)
     char opt;
     char *optarg;
{
  switch (opt) {
  case 'b':     /* border margin */
    sscanf(optarg,"%d",&border_margin);
    break;
  case 'L':     /* ignore language and magnif. */
  case 'm':
    break;
  default:
    put_msg(Err_badarg, opt, "map");
    exit(1);
  }
}

static char *
is_link(comment)
     F_comment  *comment;
{
  char *cp1, *cp2;
  Boolean have_alt = False;
  int i;

  url[0] = '\0';
  alt[0] = '\0';

  while (comment != NULL) {
    cp1 = comment->comment;
    while (isspace(*cp1)) cp1++;  /* ignore leading spaces */

    if (strncasecmp(cp1, "HREF=", 5) == 0) {
      /* comment like "HREF=url ALT=string" */

      cp2 = cp1 + 5;
      while (isspace(*cp2)) cp2++;  /* spaces */
      i = 0;
      while (isgraph(*cp2)) url[i++] = *(cp2++);  /* URL */
      url[i] = '\0';
      while (isspace(*cp2)) cp2++;  /* spaces */
      if (*cp2 != '\0') {
        if (strncasecmp(cp2, "ALT=", 4) == 0) {
          have_alt = True;
	  cp2 = cp2 + 4;
	  while (isspace(*cp2)) cp2++;  /* spaces */
	  i = 0;
	  if (*cp2 == '"') {
	    cp2++;
	    while (*cp2 != '\0' && *cp2 != '"') alt[i++] = *(cp2++);  /* text */
	  } else {
	    while (isgraph(*cp2)) alt[i++] = *(cp2++);  /* text */
	  }
	  alt[i] = '\0';
        } else {
          fprintf(stderr, "fig2dev(map): unknown attribute: %s\n", cp2);
        }
      } 
      if (!have_alt)
        fprintf(stderr, "fig2dev(map): ALT is required in HTML 3.2: %s\n", cp1);
      return cp1;
    }
    comment = comment->next;
  }
  return NULL;
}

void
add_link(char *area)
{
  if (n_links < MAX_LINKS) {
    links[n_links].url = (char *)malloc(strlen(url) + 1);
    strcpy(links[n_links].url, url);
    links[n_links].alt = (char *)malloc(strlen(alt) + 1);
    strcpy(links[n_links].alt, alt);
    links[n_links].area = (char *)malloc(strlen(area) + 1);
    strcpy(links[n_links].area, area);
    n_links++;
  } else {
    fprintf(stderr, "fig2dev(map): more than %d links in a figure\n", MAX_LINKS);
  }
}

void
genmap_start(objects)
     F_compound *objects;
{
  char *basename;
  char *ref;
  int i;

  if (to != NULL) {
    basename = (char *)malloc(strlen(to) + 1);
    strcpy(basename, to);
    for (i = strlen(basename) - 1; 0 < i; i--) {
      if (basename[i] == '.') {
        basename[i] = '\0';
        break;
      }
    }
  } else {
    basename = "imagemap";
  }

  fprintf(tfp, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Frameset//EN\">\n");
  fprintf(tfp, "<HTML>\n");
  fprintf(tfp, "<HEAD><TITLE>HTML imagemap generated by fig2dev</TITLE></HEAD>\n");
  fprintf(tfp, "<BODY>\n\n");
  fprintf(tfp, "<H1>HTML imagemap generated by fig2dev</H1>\n");
  fprintf(tfp, "<P>This file is generated by fig2dev.</P>\n");
  fprintf(tfp, "<P>You can copy the following lines into your HTML document.\n");
  fprintf(tfp, "You may need to edit the name of the image file in the first line.</P>\n");
  fprintf(tfp, "\n");
  fprintf(tfp, "<IMG SRC=\"%s.png\" USEMAP=\"#%s\">\n", basename, basename);
  fprintf(tfp, "<MAP NAME=\"%s\">\n", basename);

  ref = is_link(objects->comments);
  if (ref != NULL) {
    sprintf(buf, "<AREA COORDS=\"%d,%d,%d,%d\" %s>\n",
            XZOOM(llx), YZOOM(lly), XZOOM(urx), YZOOM(ury), ref);
    add_link(buf);
  }
}

int
genmap_end()
{
  int i;
  int len;
  char label[TEXT_LENGTH];

  for (i = n_links - 1; 0 <= i; i--) {
    fprintf(tfp, "%s", links[i].area);
  }
  fprintf(tfp, "</MAP>\n");

  fprintf(tfp, "\n");
  fprintf(tfp, "<h2>Alternative Text Links</h2>\n");
  fprintf(tfp, "<P>Because not all people can use imagemaps,\n");
  fprintf(tfp, "it is recommended to prepare an alternative way for the people\n");
  fprintf(tfp, "who can't use or don't want to use them.</P>\n");
  fprintf(tfp, "<P>Here are text the links extracted from the above imagemap.\n");
  fprintf(tfp, "If the ALT attribute (it is required in HTML 3.2) was not specified\n");
  fprintf(tfp, "in the imagemap, those URLs will be displayed\n");
  fprintf(tfp, "instead of labels specified by the ALT attributes.</P>\n");
  fprintf(tfp, "\n");
  fprintf(tfp, "<P ALIGN=\"CENTER\">\n");
  len = 0;
  for (i = n_links - 1; 0 <= i; i--) {
    if (links[i].alt[0] != '\0') strcpy(label, links[i].alt);
    else sprintf(label, "<TT>%s</TT>", links[i].url);
    fprintf(tfp, "%s<A HREF=%s>%s</A>\n",
	    (len == 0) ? "" : " | ", links[i].url, label);
	    
    len = len + strlen(label) + 3;
    if (50 < len) {
      fprintf(tfp, "<BR>\n");
      len = 0;
    }
  }
  fprintf(tfp, "</P>\n");

  fprintf(tfp, "\n");
  fprintf(tfp, "</BODY>\n");
  fprintf(tfp, "</HTML>\n");

  /* all ok */
  return 0;
}

void
genmap_arc(a)
     F_arc      *a;
{
  char *ref;
  int cx, cy, sx, sy, ex, ey;
  double r;
  double sa, ea;
  double alpha;

  ref = is_link(a->comments);
  if (ref != NULL) {
    cx = XZOOM(a->center.x);
    cy = YZOOM(a->center.y);
    sx = XZOOM(a->point[0].x);
    sy = YZOOM(a->point[0].y);
    ex = XZOOM(a->point[2].x);
    ey = YZOOM(a->point[2].y);

    if (a->direction == 0) {
      sa = atan2((double)(sy - cy), (double)(sx - cx));
      ea = atan2((double)(ey - cy), (double)(ex - cx));
    } else {
      ea = atan2((double)(sy - cy), (double)(sx - cx));
      sa = atan2((double)(ey - cy), (double)(ex - cx));
    }
    if (ea < sa) ea = ea + 2 * M_PI;

    r = sqrt((double)((sx - cx) * (sx - cx)
		      + (sy - cy) * (sy - cy))) * ARC_EXPAND + 1.0;

    sprintf(buf, "<AREA SHAPE=\"poly\" COORDS=\"");
    if (a->type == T_PIE_WEDGE_ARC)
      sprintf(&buf[strlen(buf)], "%d,%d,", cx, cy);
    for (alpha = sa; alpha < (ea - ARC_STEP / 4); alpha += ARC_STEP) {
      if (alpha != sa) sprintf(&buf[strlen(buf)], ",");
      sprintf(&buf[strlen(buf)], "%d,%d",
	      round(cx + r * cos(alpha)), round(cy + r * sin(alpha)));
    }
    sprintf(&buf[strlen(buf)], ",%d,%d",
	    round(cx + r * cos(ea)), round(cy + r * sin(ea)));
    sprintf(&buf[strlen(buf)], "\" %s>\n", ref);
    add_link(buf);
  }
}

void
genmap_ellipse(e)
     F_ellipse  *e;
{
  char *ref;
  int x0, y0;
  double rx, ry;
  double angle, theta;

  ref = is_link(e->comments);
  if (ref != NULL) {
    x0 = XZOOM(e->center.x);
    y0 = YZOOM(e->center.y);
    rx = mag * e->radiuses.x / fscale;
    ry = mag * e->radiuses.y / fscale;
    angle = - e->angle;
    if (e->radiuses.x == e->radiuses.y) {
      sprintf(buf, "<AREA SHAPE=\"circle\" COORDS=\"%d,%d,%d\" %s>\n",
	      x0, y0, round(rx) + 1, ref);
      add_link(buf);
    } else {
      rx = rx * ARC_EXPAND + 1.0;
      ry = ry * ARC_EXPAND + 1.0;
      sprintf(buf, "<AREA SHAPE=\"poly\" COORDS=\"");
      for (theta = 0.0; theta < 2.0 * M_PI; theta += ARC_STEP) {
	if (theta != 0.0) sprintf(&buf[strlen(buf)], ",");
	sprintf(&buf[strlen(buf)], "%d,%d",
		round(x0 + cos(angle) * rx * cos(theta)
		      - sin(angle) * ry * sin(theta)),
		round(y0 + sin(angle) * rx * cos(theta)
		      + cos(angle) * ry * sin(theta)));
      }
      sprintf(&buf[strlen(buf)], "\" %s>\n", ref);
      add_link(buf);
    }
  }
}

void
genmap_line(l)
     F_line     *l;
{
  char *ref;
  int xmin, xmax, ymin, ymax;
  int x, y, last_x, last_y;
  F_point *p;

  ref = is_link(l->comments);
  if (ref != NULL) {
    switch (l->type) {
    case T_BOX:
    case T_ARC_BOX:
    case T_PIC_BOX:
      xmin = xmax = l->points->x;
      ymin = ymax = l->points->y;
      for (p = l->points->next; p != NULL; p = p->next) {
        if (p->x < xmin) xmin = p->x;
        if (xmax < p->x) xmax = p->x;
        if (p->y < ymin) ymin = p->y;
        if (ymax < p->y) ymax = p->y;
      }
      sprintf(buf, "<AREA COORDS=\"%d,%d,%d,%d\" %s>\n",
              XZOOM(xmin), YZOOM(ymin), XZOOM(xmax), YZOOM(ymax), ref);
      add_link(buf);
      break;
    case T_POLYLINE:
    case T_POLYGON:
      sprintf(buf, "<AREA SHAPE=\"poly\" COORDS=\"");
      for (p = l->points; (l->type==T_POLYLINE? p: p->next) != NULL; p = p->next) {
        x = XZOOM(p->x);
        y = YZOOM(p->y);
        if (p == l->points
            || MIN_DIST < abs(last_x - x) || MIN_DIST < abs(last_y - y)) {
          if (sizeof(buf) < strlen(buf) + 100) {
            fprintf(stderr, "fig2dev(map): too complex POLYLINE... ignored\n");
            return;
          } else {
            if (p != l->points) sprintf(&buf[strlen(buf)], ",");
            sprintf(&buf[strlen(buf)], "%d,%d", x, y);
            last_x = x;
            last_y = y;
          }
        }
      }
      sprintf(&buf[strlen(buf)], "\" %s>\n", ref);
      add_link(buf);
      break;
    }
  }
}

void
genmap_text(t)
     F_text     *t;
{
  char *ref;

  ref = is_link(t->comments);
  if (ref != NULL) {
    fprintf(stderr, "fig2dev(map): TEXT can't be used as link region\n");
  }
}

struct driver dev_map = {
        genmap_option,
        genmap_start,
	gendev_null,
        genmap_arc,
        genmap_ellipse,
        genmap_line,
        gendev_null,
        genmap_text,
        genmap_end,
        INCLUDE_TEXT
};

