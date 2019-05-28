#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"


#define PAGE_SIZE 4096
// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header
{
	struct
	{
		union header *ptr;
		uint size;
	} s;
	Align x;
};

//added
typedef struct pg_hdr{
	struct pg_hdr *next;
}pg_header;

typedef union header Header;

static Header base;
static Header *freep;

//added
static pg_header *first_head;

void
free(void *ap)
{
	if(checkpg(ap)){
		pfree(ap);
	}
	Header *bp, *p;

	bp = (Header *) ap - 1;
	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break;
	if (bp + bp->s.size == p->s.ptr)
	{
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else
		bp->s.ptr = p->s.ptr;
	if (p + p->s.size == bp)
	{
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else
		p->s.ptr = bp;
	freep = p;
}



int pfree(void * ap){
	
	if(!checkpg(ap)){
		return -1;
	}
	
	freepm(ap);
	pg_header * head= (pg_header*)ap;
	if(!first_head){
		//printf(1,"69\n");
		head->next =0;
		//printf(1,"71\n");
	}else{
		//printf(1,"73\n");
		head->next = first_head;
		//printf(1,"75\n");
	}
	first_head =head;
	return 1;
}


int protect_page(void * ap){
	//THIS PAGE CREATED USING PMALLOC AND ITS ALIGND
	if(checkpg(ap)  && (uint)ap%PAGE_SIZE ==0 ){
		proton(ap);
		return 1;
	}
	return -1;
}

static Header *
morecore(uint nu)
{
	char *p;
	Header *hp;

	if (nu < 4096)
		nu = 4096;
	p = sbrk(nu * sizeof(Header));
	if (p == (char *) -1)
		return 0;
	hp = (Header *) p;
	hp->s.size = nu;
	free((void *) (hp + 1));
	return freep;
}

void *
malloc(uint nbytes)
{
	Header *p, *prevp;
	uint nunits;

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
	if ((prevp = freep) == 0)
	{
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}
	for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr)
	{
		if (p->s.size >= nunits)
		{
			if (p->s.size == nunits)
				prevp->s.ptr = p->s.ptr;
			else
			{
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;
			return (void *) (p + 1);
		}
		if (p == freep)
			if ((p = morecore(nunits)) == 0)
				return 0;
	}
}

void *pmalloc()
{
	void *p;
	
	if(!first_head)
		p = sbrk(PAGE_SIZE);
	else
	{
		p =(void*)first_head;
		first_head = first_head->next;
	}

	pgon(p);
	return p;
}
