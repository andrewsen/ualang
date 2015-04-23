//
//  Program.cs
//
//  Author:
//       Andrew Senko <andrewsen98@gmail.com>
//
//  Copyright (c) 2014 Andrew Senko
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
using System;
using System.Collections.Generic;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CompilerClasses;
using Gnu.Getopt;
using Language;
using System.Reflection;

namespace Translator
{
    enum SyntaxLang
    {        English = 0,
        Ukrainian = 1

    }

    static class CommandArgs
    {
        public static SyntaxLang lang = SyntaxLang.English;
        public static bool newTac = true;
        public static bool noInternal = false;
        public static bool asmOnly = false;
        public static bool lib = false;        
        public static string outFile = null;
        public static string source = null;
    }

    class Program
    {
        static int Main(string[] args)
        {

            try
            {
                /*
                 * С 1.03 конкурс. Представить работу. 
                 * ~ 25.02 зайти на it-arena.org
                 * Указать целевую машину на первом слайде
                 */
                LongOpt[] longopts = new LongOpt[7];

                //StringBuilder sb = new StringBuilder();
                longopts[0] = new LongOpt("help", Argument.No, null, 'h');
                longopts[1] = new LongOpt("version", Argument.No, null, 'v');
                longopts[2] = new LongOpt("asm", Argument.No, null, 'S');
                longopts[3] = new LongOpt("no-internal", Argument.No, null, 0); 
                longopts[4] = new LongOpt("lib", Argument.No, null, 'L');
                longopts[5] = new LongOpt("locale", Argument.Required, null, 'l'); 
                longopts[6] = new LongOpt("out", Argument.Required, null, 'o'); 

                Getopt g = new Getopt("comp", args, "l:o:hvS", longopts);
                g.Opterr = false; // We'll do our own error handling
                int c;                
                string arg;
                while ((c = g.getopt()) != -1)
                    switch (c)
                    {
                        case 0:
                            CommandArgs.noInternal = true;
                            break;
                        case 'h':
                            Console.WriteLine("uasge: " + Environment.CommandLine + " [flags] <sources>");
                            break;
                        case 'v':
                            Console.WriteLine("version: indev");
                            break;
                        case 'S':
                            CommandArgs.asmOnly = true;
                            break;
                        case 'l':
                            if(g.Optarg.ToLower() == "english" || g.Optarg == "англійська" || g.Optarg == "Англійська")
                                CommandArgs.lang = SyntaxLang.English;
                            else if(g.Optarg.ToLower() == "ukrainian" || g.Optarg == "українська" || g.Optarg == "Українська")
                                CommandArgs.lang = SyntaxLang.Ukrainian;
                            else
                                Console.WriteLine("Unknown locale: " + g.Optarg);
                            break;
                        case 'o':
                            CommandArgs.outFile = g.Optarg;
                            break;
                        case 'L':
                            CommandArgs.lib = true;
                            break;

                        case ':':
                            Console.WriteLine("You need an argument for option " +
                                (char)g.Optopt);
                            break;

                        case '?':
                            Console.WriteLine("The option '" + (char)g.Optopt + 
                                "' is not valid");
                            break;

                        default:
                            //Console.WriteLine("getopt() returned " + c);
                            break;
                    }
                                
                if (args.Length < 1)
                {
                    Console.WriteLine("uasge: " + Environment.CommandLine + " <file.mc>");
                    return 1;
                }
                List<string> files = new List<string>();

                for (int i = g.Optind; i < args.Length ; i++)
                {
                    files.Add(args[i]);
                }

                    //string file = CommandArgs.source = args[i];
                
                string wd = Environment.CurrentDirectory + "/";
                string name = CommandArgs.outFile ?? wd + Path.GetFileNameWithoutExtension(files[0]) + ".vas";

                Compiler compiler = new Compiler(files.ToArray(), name);
                
                compiler.Compile();

                Console.WriteLine("Compiled successfully!");
                Console.WriteLine("Output: " + wd + name + ".vas");
                Console.WriteLine("Three address code: " + wd + name + ".tac");
                //}               
                return 0;
            }
            catch (CompilerException ce)
            {
                Console.Write("Compilation error: ");
                Console.WriteLine(ce.Type.ToString());
                Console.WriteLine(ce.Message);
                Console.WriteLine(ce.What + " at " + ce.Where);
                return 1;
            }
        }
    }
}

