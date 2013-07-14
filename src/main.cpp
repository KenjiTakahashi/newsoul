/*  newsoul - A SoulSeek client written in C++
    Copyright (C) 2006-2007 Ingmar K. Steen (iksteen@gmail.com)
    Copyright 2008 little blue poney <lbponey@users.sourceforge.net>
    Karol 'Kenji Takahashi' Woźniak © 2013

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#include "newsoul.h"

/* Returns 0 if newsoul is already running, 1 otherwise. */
int get_lock(void)
{
# ifdef HAVE_FCNTL_H
  struct flock fl;
  int fdlock;

  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 1;

  if((fdlock = open("/tmp/newsoul.lock", O_WRONLY|O_CREAT, 0666)) == -1)
    return 0;

  if(fcntl(fdlock, F_SETLK, &fl) == -1)
    return 0;

# endif // HAVE_FCNTL_H

  return 1;
}

int main(int argc, char *argv[]) {
  if(!get_lock())
  {
    std::cerr << "newsoul already running!" << std::endl;
    return 1;
  }

  /* Enable various interesting logging domains. */
  NNLOG.logEvent.connect(new NewNet::ConsoleOutput);
  NNLOG.enable("ALL");

  /* Check size of off_t */
  if(sizeof(off_t) < 8)
  {
    NNLOG("newsoul.warn", "Warning: No large file support. This means you can't download files larger than 4GB.");
  }

  /* Create our newsoul Daemon instance. */
  newsoul::Newsoul app;
  return app.run(argc, argv);
}
