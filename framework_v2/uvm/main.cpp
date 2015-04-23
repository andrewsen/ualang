/*
         ___________________________________________________________________________________________________________________________
        /  ______   __________    _____         _______             __      ___     ___      ____         _______      ______      /
       /  /  ___/  /___   ___/   /     |       / __    /           |  |    /   |   /  /    /     |       / __    /    /  ___/     /
      /  /  /         /  /      /  /|  |      / /__/  /            |  |   /    |  /  /    /  /|  |      / /__/  /    /  /        /
     /   \  \        /  /      /  /_|  |     /     __/             |  |  /  |  | /  /    /  /_|  |     /     __/     \  \       /
    /     \  \      /  /      /  /__|  |    /  /\  \               |  | /  /|  |/  /    /  /__|  |    /  /\  \        \  \     /
   /   ___/  /     /  /      /  /   |  |   /  /  \  \              |  |/  / |     /    /  /   |  |   /  /  \  \    ___/  /    /
  /   /_____/     /__/      /__/    |__|  /__/    \__\             |_____/  |____/    /__/    |__|  /__/    \__\  /_____/    /
 /__________________________________________________________________________________________________________________________/

                                                                                               __
                                                                                              /\ \
                                                                    ________--------____      \*\ \
                                                    ________--------                    ---___ \$\=\
                                    ________--------                         -=               --\*\ \_
                    ________--------                  -=                                       //\$\=\---____
    ________--------          -=                                                              ////\*\ \      ---
   <==========================================================================================/////\$\=\=========
    ----____..................................................................................//////\*\_\......_> **
            ----____.............-=..................................................................\/_/...._-
                    ----..................................................................................._-_] **
                            ----____.....................-=.............................................._-___] **
                                    ----____..........................................................._-
                                            ----____.............................-=.................._-
                                                    ----____......................................._-_] **
                                                            ----____............................._-___] **
                                                                    ----____.................. _-
                                                                            ----____ ........_-
                                                                                    ----____-

                                                                                                           STAR DESTROYER
*/
#include <iostream>
#include <csignal>
#include "runtime.h"

using namespace std;

int main(int argc, char* argv[])
{
    if(argc == 1)
    {
        cout << "Usage: " << argv[0] << " <module.sem>\n";
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = [&](int signal, siginfo_t *si, void *arg)
    {
        cout << "Segfault occured when trying to access memory at 0x" << hex << (size_t)si->si_addr << dec << endl;
        if(si->si_code == SEGV_ACCERR)
            cout << "Invalid permissions\n";
        else
            cout << "Address not mapped\n";
        exit(EXIT_FAILURE);
    };
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);

    try {
        Runtime rt;
        rt.Create(argv[1]);
        rt.Load();
        //rt.Start(rt.main, --argc, ++argv);
        //char* args[] = {"/home/senko/qt/ualang/framework_v2/uasm/minic.sem"};
        rt.Start(rt.FindFunctionId("main"), 0, nullptr);
        rt.Unload();
        if(rt.HasReturnCode()) return rt.GetReturnCode();
    }
    catch(Module::InvalidMagicException ime) {
        cerr << ime.What() << endl << hex << ime.GetMagic() << dec << endl;
        cerr << "In file: " << argv[1] << endl;
    }
    catch(Runtime::RtExType rte) {
        switch (rte) {
            case Runtime::StackOverflow:
                cerr << "Stack overflow" << endl;
                break;
            default:
                cerr << "Unknown error" << endl;
                break;
        }
    }

    return 0;
}
