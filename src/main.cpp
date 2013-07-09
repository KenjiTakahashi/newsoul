/*  newsoul - A SoulSeek client written in C++
    Copyright (C) 2006-2007 Ingmar K. Steen (iksteen@gmail.com)
    Copyright 2008 little blue poney <lbponey@users.sourceforge.net>

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
#include "ifacemanager.h"
#include <signal.h>

/* Global reference to the newsoul instance. */
static NewNet::RefPtr<newsoul::Newsoul> app;

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

/* Our signal handler, stop the reactor if we receive a HUP or INT signal. */
static void newsoul_signal_handler(int signal)
{
    if (signal == SIGINT) {
        NNLOG("newsoul.debug", "Trapped signal %i. Stopping the reactor.", signal);
        app->reactor()->stop();
    }
#ifndef WIN32
    else if (signal == SIGHUP) {
        NNLOG("newsoul.debug", "Trapped signal %i. Reloading shares.", signal);
        app->LoadShares();
    }
    else if (signal == SIGALRM) {
        NNLOG("newsoul.debug", "Trapped signal %i. Trying to reconnect to server.", signal);
        if (!app->server()->loggedIn())
            app->server()->connect();
    }
#endif // WIN32


  /* Reconnect signal handlers for HUP, ALRM and INT signals.
     Some Unix disconnect signals after each call */
#ifndef WIN32
  ::signal(SIGHUP, &newsoul_signal_handler);
  ::signal(SIGALRM, &newsoul_signal_handler);
#endif // WIN32
  ::signal(SIGINT, &newsoul_signal_handler);
}

/* Timeout callback to connect to the server. Not really required, could
   call connect() directly from main() but it can be useful for debugging
   purposes. */
class AutoConnectCallback : public NewNet::Reactor::Timeout::Callback
{
public:
  virtual void operator()(long)
  {
    app->server()->connect();
  }
};

class KeySetCallback : public NewNet::Event<const newsoul::ConfigManager::ChangeNotify *>::Callback
{
public:
  virtual void operator()(const newsoul::ConfigManager::ChangeNotify * notice)
  {
    if(notice->domain == "newsoul.debug")
    {
      if(app->config()->getBool(notice->domain, notice->key))
        NNLOG.enable(notice->key);
      else
        NNLOG.disable(notice->key);
    }
  }
};

class KeyRemovedCallback : public NewNet::Event<const newsoul::ConfigManager::RemoveNotify *>::Callback
{
public:
  virtual void operator()(const newsoul::ConfigManager::RemoveNotify * notice)
  {
    if(notice->domain == "newsoul.debug")
      NNLOG.disable(notice->key);
  }
};

int main(int argc, char ** argv)
{
  if(!get_lock())
  {
    std::cerr << "newsoul already running!" << std::endl;
    return 1;
  }

  std::string version("newsoul :: Version 0.3.0 :: newsoul Daemon Plus");

#ifndef WIN32
  /* Load the configuration from ~/.newsoul/config.xml. */
  char *home = getenv("HOME");
  std::string configPath;
  if(home != NULL) {
      configPath = std::string(std::string(home) + "/.newsoul/config.xml");
  }
#else
  /* Load the configuration from %APPDIR%\Newsoul\config.xml. */
  std::string configPath(getConfigPath("Newsoul") + "\\config.xml");
#endif // WIN32

    bool fullDebug = false;

  for(int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if(arg == "--config" || arg == "-c") {
      if(i + 1 < argc) {
        configPath = argv[i+1];
      } else {
        std::cerr << "Missing config file path, bailing out." << std::endl;
        return -1;
      }
    } else if(arg == "--version" || arg == "-V" ) {
      std::cout << version << std::endl;
      return 0;
    } else if(arg == "--debug" || arg == "-d" ) {
      fullDebug = true;
    } else if(arg == "--help" || arg == "-h") {
      std::cout << version << std::endl;
      std::cout << "Syntax: newsoul [options]" << std::endl << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "-c --config <config file>\tUse alternative config file" << std::endl;
      std::cout << "-h --help\t\t\tDisplay this message and quit" << std::endl;
      std::cout << "-d --debug\t\t\tDisplay debugging messages" << std::endl;
      std::cout << "-V --version\t\t\tDisplay newsoul version and quit" << std::endl << std::endl;
      std::cout << "Signals:" << std::endl;
      std::cout << "kill -HUP \tReload Shares Database(s)" << std::endl;
      std::cout << "kill -ALRM \tReconnect to Server" << std::endl;
      return 0;
    }
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
  app = new newsoul::Newsoul();

  /* Connect our config watchers. */
  app->config()->keySetEvent.connect(new KeySetCallback);
  app->config()->keyRemovedEvent.connect(new KeyRemovedCallback);

  if(!app->config()->load(configPath))
  {
    NNLOG("newsoul.config.warn", "Failed to load configuration, bailing out.");
    return -1;
  }

  /* Disable the debug override. */
  if(!app->config()->getBool("newsoul.debug", "ALL") && !fullDebug)
    NNLOG.disable("ALL");

  /* Load the shares database. */
  app->LoadShares();
  app->LoadDownloads();

  /* Connect signal handlers for HUP, ALRM and INT signals. */
#ifndef WIN32
  signal(SIGHUP, &newsoul_signal_handler);
  signal(SIGALRM, &newsoul_signal_handler);
#endif // WIN32
  signal(SIGINT, &newsoul_signal_handler);

  /* Add a timeout callback: Wait 5 seconds, then connect to the server. */
  app->reactor()->addTimeout(5000, new AutoConnectCallback);

  /* Start the reactor. This drives the daemon. */
  app->reactor()->run();
  app->downloads()->saveDownloads();

  return 0;
}
