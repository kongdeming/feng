/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

/* here we have some utils to handle the ResourceDescr and MediaDescr structire
 * types in order to simplify its usage to the mediathread library user */

#include <fenice/demuxer.h>

/* --- ResourceInfo wrapper functions --- */

inline time_t r_descr_last_change(ResourceDescr *r_descr)
{
	return r_descr ? r_descr->last_change : 0;
}

inline char *r_descr_mrl(ResourceDescr *r_descr)
{
	return (r_descr && r_descr->info) ? r_descr->info->mrl : NULL;
}

inline char *r_descr_twin(ResourceDescr *r_descr)
{
	// use this if 'twin' become a char pointer
	// return (r_descr && r_descr->info) ? r_descr->info->twin : NULL;
	return (r_descr && r_descr->info && *r_descr->info->twin) ? r_descr->info->twin : NULL;
}

inline char *r_descr_multicast(ResourceDescr *r_descr)
{
	// use this if 'multicast' become a char pointer
	// return (r_descr && r_descr->info) ? r_descr->info->multicast : NULL;
	return (r_descr && r_descr->info && *r_descr->info->multicast) ? r_descr->info->multicast : NULL;
}

inline char *r_descr_ttl(ResourceDescr *r_descr)
{
	// use this if 'ttl' become a char pointer
	// return (r_descr && r_descr->info) ? r_descr->info->ttl : NULL;
	return (r_descr && r_descr->info && *r_descr->info->ttl) ? r_descr->info->ttl : NULL;
}