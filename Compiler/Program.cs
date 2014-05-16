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
using Language;

namespace Translator
{
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

                if(args.Length != 1) {
                    Console.WriteLine("uasge: " + Environment.CommandLine + " <file.mc>");
                    return 1;
                }

                string file = args[0];
                string name = Path.GetFileNameWithoutExtension(args[0]);
                string wd = Environment.CurrentDirectory + "/";

                Compiler compiler = new Compiler(file, wd + name + ".vas");
                
                compiler.Compile();

                Console.WriteLine("Compiled successfully!");
                Console.WriteLine("Output: " + wd + name + ".vas");
                Console.WriteLine("Three address code: " + wd + name + ".tac");
                                
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

