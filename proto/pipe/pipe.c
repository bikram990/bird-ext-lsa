/*
 *	BIRD -- Table-to-Table Routing Protocol a.k.a Pipe
 *
 *	(c) 1999 Martin Mares <mj@ucw.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#define LOCAL_DEBUG

#include "nest/bird.h"
#include "nest/iface.h"
#include "nest/protocol.h"
#include "nest/route.h"
#include "conf/conf.h"
#include "filter/filter.h"

#include "pipe.h"

static void
pipe_send(struct pipe_proto *p, rtable *dest, net *n, rte *new, rte *old)
{
  net *nn;
  rte *e;
  rta a;

  if (dest->pipe_busy)
    {
      log(L_ERR "Pipe loop detected when sending %I/%d to table %s",
	  n->n.prefix, n->n.pxlen, dest->name);
      return;
    }
  nn = net_get(dest, n->n.prefix, n->n.pxlen);
  if (new)
    {
      memcpy(&a, new->attrs, sizeof(rta));
      a.proto = &p->p;
      a.source = RTS_PIPE;
      a.aflags = 0;
      e = rte_get_temp(&a);
      e->net = nn;
    }
  else
    e = NULL;
  dest->pipe_busy = 1;
  rte_update(dest, nn, &p->p, e);
  dest->pipe_busy = 0;
}

static void
pipe_rt_notify_pri(struct proto *P, net *net, rte *new, rte *old, ea_list *tmpa)
{
  struct pipe_proto *p = (struct pipe_proto *) P;

  DBG("PIPE %c> %I/%d\n", (new ? '+' : '-'), net->n.prefix, net->n.pxlen);
  pipe_send(p, p->peer, net, new, old);
}

static void
pipe_rt_notify_sec(struct proto *P, net *net, rte *new, rte *old, ea_list *tmpa)
{
  struct pipe_proto *p = ((struct pipe_proto *) P)->phantom;

  DBG("PIPE %c< %I/%d\n", (new ? '+' : '-'), net->n.prefix, net->n.pxlen);
  pipe_send(p, p->p.table, net, new, old);
}

static int
pipe_import_control(struct proto *P, rte **ee, ea_list **ea, struct linpool *p)
{
  struct proto *pp = (*ee)->attrs->proto;

  if (pp == P || pp == &((struct pipe_proto *) P)->phantom->p)
    return -1;	/* Avoid local loops automatically */
  return 0;
}

static int
pipe_start(struct proto *P)
{
  struct pipe_proto *p = (struct pipe_proto *) P;
  struct pipe_proto *ph;
  struct announce_hook *a;

  /*
   *  Create a phantom protocol which will represent the remote
   *  end of the pipe (we need to do this in order to get different
   *  filters and announce functions and it unfortunately involves
   *  a couple of magic trickery).
   */
  ph = mb_alloc(P->pool, sizeof(struct pipe_proto));
  memcpy(ph, p, sizeof(struct pipe_proto));
  p->phantom = ph;
  ph->phantom = p;
  ph->p.rt_notify = pipe_rt_notify_sec;
  ph->p.proto_state = PS_UP;
  ph->p.core_state = ph->p.core_goal = FS_HAPPY;
  ph->p.in_filter = ph->p.out_filter = FILTER_ACCEPT;	/* We do all filtering on the local end */

  /*
   *  Connect the phantom protocol to the peer routing table, but
   *  keep it in the list of connections of the primary protocol,
   *  so that it gets disconnected at the right time and we also
   *  get all routes from both sides during the feeding phase.
   */
  a = proto_add_announce_hook(P, p->peer);
  a->proto = &ph->p;

  return PS_UP;
}

static struct proto *
pipe_init(struct proto_config *C)
{
  struct pipe_config *c = (struct pipe_config *) C;
  struct proto *P = proto_new(C, sizeof(struct pipe_proto));
  struct pipe_proto *p = (struct pipe_proto *) P;

  p->peer = c->peer->table;
  P->rt_notify = pipe_rt_notify_pri;
  P->import_control = pipe_import_control;
  return P;
}

static void
pipe_postconfig(struct proto_config *C)
{
  struct pipe_config *c = (struct pipe_config *) C;

  if (!c->peer)
    cf_error("Name of peer routing table not specified");
  if (c->peer == C->table)
    cf_error("Primary table and peer table must be different");
}

struct protocol proto_pipe = {
  name:		"Pipe",
  postconfig:	pipe_postconfig,
  init:		pipe_init,
  start:	pipe_start,
};