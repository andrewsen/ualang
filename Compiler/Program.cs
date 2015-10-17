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
using System.Reflection;

using CompilerClasses;
using Gnu.Getopt;
using Language;

namespace Translator
{
    enum SyntaxLang
    {        
        English = 0,
        Ukrainian = 1
    }

    static class CommandArgs
    {
        public static bool saveAsm= false;
        public static bool writeTac = false;
        public static bool newTac = true;
        public static bool noInternal = false;
        public static bool onlyProduceAsmSource = false;
        public static bool runAssembler = false;
        public static bool lib = false;        
        public static string outTacFile = null;
        public static string outFile = null;
        public static string source = null;
        public static SyntaxLang lang = SyntaxLang.English;
    }

    class Program
    {
        //5659 lines
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
                longopts[2] = new LongOpt("asm-only", Argument.No, null, 'S');
                longopts[2] = new LongOpt("assemble", Argument.No, null, 'A');
                longopts[3] = new LongOpt("no-internal", Argument.No, null, 0); 
                longopts[4] = new LongOpt("lib", Argument.No, null, 'L');
                longopts[4] = new LongOpt("debug", Argument.No, null, 'D');
                longopts[4] = new LongOpt("tac", Argument.Optional, null, 't');
                longopts[5] = new LongOpt("locale", Argument.Required, null, 'l'); 
                longopts[6] = new LongOpt("out", Argument.Required, null, 'o'); 

                Getopt g = new Getopt("comp", args, "l:o:htvSA", longopts);
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
                            CommandArgs.onlyProduceAsmSource = true;
                            CommandArgs.saveAsm = true;
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
                        case 't':
                            CommandArgs.writeTac = true;
                            if(g.Optarg != null)
                                CommandArgs.outTacFile = g.Optarg;
                            else
                                CommandArgs.outTacFile = null;
                            break;
                        case 'A':
                            CommandArgs.runAssembler = true;
                            CommandArgs.saveAsm = true;
                            break;
                        case 'L':
                            CommandArgs.lib = true;
                            break;
                        case 'D':
                            CommandArgs.writeTac = true;
                            CommandArgs.saveAsm = true;
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
                    Console.WriteLine("uasge: " + Environment.CommandLine + " <files>");
                    return 1;
                }
                List<string> files = new List<string>();

                for (int i = g.Optind; i < args.Length ; i++)
                {
                    files.Add(args[i]);
                }

                if(CommandArgs.runAssembler)
                {
                    string asm_args = "";
                    foreach (string f in files)
                        asm_args += f + " ";
                    return Assemble("./uasm", asm_args, true);
                }

                    //string file = CommandArgs.source = args[i];
                
                string wd = Environment.CurrentDirectory + "/";
                string name = CommandArgs.outFile;
                bool fullpath = false;
                if(CommandArgs.outFile == null)
                {
                    name = wd + Path.GetFileNameWithoutExtension(files[0]) + ".vas";
                    fullpath = true;
                }

                Compiler compiler = new Compiler(files.ToArray(), name);
                
                compiler.Compile();

                Console.WriteLine("Compiled successfully!");
                Console.WriteLine("Output: " + (fullpath ? "" : wd) + CommandArgs.outFile);
                if(CommandArgs.writeTac)
                    Console.WriteLine("Three address code: " + (fullpath ? "" : wd) + (CommandArgs.outTacFile ?? CommandArgs.outFile + ".tac"));

                if(CommandArgs.onlyProduceAsmSource)
                    return 0;
                Console.WriteLine("Compilling...");

                /*  STARTING ASSEMBLER */
                return Assemble("./uasm", (fullpath ? "" : wd) + CommandArgs.outFile, fullpath);

            }
            catch (IOException ce)
            {
                Console.Write("Compilation error: ");
                //Console.WriteLine(ce.Type.ToString());
                Console.WriteLine(ce.Message);
                //Console.WriteLine(ce.What + " at " + ce.Where);
                return 1;
            }
        }

        static int Assemble(string prog, string args, bool fullpath)
        {
            ProcessStartInfo start = new ProcessStartInfo();
            start.Arguments = args; 
            start.FileName = prog;

            int exitCode;

            Console.WriteLine("Starting " + start.FileName + " " + start.Arguments);
            using (Process proc = Process.Start(start))
            {
                proc.WaitForExit();

                exitCode = proc.ExitCode;
            }

            //TODO: Finish later
            //if (!CommandArgs.saveAsm)
            //{
            //    File.Delete(CommandArgs.outFile);
            //}

            return exitCode;
        }
    }
}

