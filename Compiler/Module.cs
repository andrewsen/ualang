//
//  Module.cs
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
using System.IO;
using Translator;
using CompilerClasses;

namespace Compiler
{
    public static class FSExt
    {        
        public static int ReadInt(this FileStream fs)
        {
            byte[] offset = new byte[4];
            fs.Read(offset, 0, 4);
            return BitConverter.ToInt32(offset, 0);
        }
        public static uint ReadUInt(this FileStream fs)
        {
            byte[] offset = new byte[4];
            fs.Read(offset, 0, 4);
            return BitConverter.ToUInt32(offset, 0);
        }
        public static bool ReadBool(this FileStream fs)
        {
            byte[] offset = new byte[1];
            fs.Read(offset, 0, 1);
            return BitConverter.ToBoolean(offset, 0);
        }
        public static string ReadCStr(this FileStream fs)
        {
            string str = "";

            for (char ch = (char)fs.ReadByte(); ch != '\0'; ch = (char)fs.ReadByte())
            {
                str += ch;
            }
            return str;
        }
    }

    public enum HeaderType : byte {
        Bytecode=1, Strings, Imports, Metadata, Functions, Globals, User
    }
    
    public enum CompiledType : byte {
        VOID,
        UI8,
        I16,
        UI32,
        I32,
        UI64,
        I64,
        BOOL,
        DOUBLE,
        UTF8
    }

    public struct SegmentHeader
    {
        public string Name;
        public uint StartPosition;
        public uint EndPosition;
        public HeaderType Type;
    }

    /*public struct Function
    {
        public string Signature;
        public CompiledType RetType;
        public List<CompiledType> Args;
        public bool isPrivate;
        public bool imported;

        /*public Function(string s, CompiledType t, List<CompiledType> a)
        {
            Signature = s;
            RetType = t;
            Args = a;
        }
    }*/

    internal struct ModuleGlobal
    {
        public string Name;
        public DataTypes Type;
        public bool isPrivate;
    }

    public class Module
    {
        public readonly uint MODULE_SIGN = 0x4153DEC0;
        public static readonly string VM_INTERNAL = "::vm.internal";

        private string path, name;
        Translator.Compiler compiler;

        public List<string> SearchPath;
        
        private List<SegmentHeader> headers;
        internal List<Function> functions;
        private List<ModuleGlobal> globals;

        [Obsolete("Don't use this", true)]
        public static Module CreateVmInternal()
        {

            Module im = new Module(null);
            /*im.functions = new List<Function>();
            im.functions.Add(new Function("::vm.internal", "print", DataTypes.Void, new List<VarType> {new VarType(DataTypes.String)}));
            im.functions.Add(new Function("::vm.internal", "print", DataTypes.Void, new List<VarType> {new VarType(DataTypes.Int)}));
            im.functions.Add(new Function("::vm.internal", "print", DataTypes.Void, new List<VarType> {new VarType(DataTypes.Double)}));
            im.functions.Add(new Function("::vm.internal", "reads", DataTypes.String, new List<VarType> {}));
            im.functions.Add(new Function("::vm.internal", "readi", DataTypes.Int, new List<VarType> {}));
            im.functions.Add(new Function("::vm.internal", "readd", DataTypes.Double, new List<VarType> {}));
            im.functions.Add(new Function("print", CompiledType.VOID, new List<CompiledType> {CompiledType.UTF8}));
            im.functions.Add(new Function("print", CompiledType.VOID, new List<CompiledType> {CompiledType.I32}));
            im.functions.Add(new Function("print", CompiledType.VOID, new List<CompiledType> {CompiledType.DOUBLE}));
            im.functions.Add(new Function("reads", CompiledType.UTF8, new List<CompiledType> {}));
            im.functions.Add(new Function("readi", CompiledType.I32, new List<CompiledType> {}));
            im.functions.Add(new Function("readd", CompiledType.DOUBLE, new List<CompiledType> {}));*/

            return im;
        }

        public Module(Translator.Compiler comp)
        {
            path = "";
            compiler = comp;
            headers = new List<SegmentHeader>();
            SearchPath = new List<string>();
            SearchPath.Add(Environment.CurrentDirectory + "/");
            //SearchPath.Add(Path.GetFullPath("~/.local/uvm/modules/"));
            SearchPath.Add("/usr/share/usdk/modules/");
        }

        public Module Load(string p)
        {
            name = p;
            if (path != "")
                return this;
            //path = p;
            SearchPath.Add(Directory.GetParent(CommandArgs.source).FullName + "/");
            if (p.StartsWith("/") || p.StartsWith("./") || p.StartsWith("../"))
            {
                path = Path.GetFullPath(p);
                if (!File.Exists(path))
                    throw new ModuleNotFoundException(path);
            }
            else
            {
                foreach (string sp in SearchPath)
                {
                    Console.WriteLine("Searching in " + sp + p);
                    if (File.Exists(sp + p))
                    {
                        path = sp + p;
                        break;
                    }
                }
            }
            if(path == "")                
                throw new ModuleNotFoundException(path);
            Console.WriteLine("Module found: " + path);
            
            //Console.WriteLine(File.ReadAllText(path));
            FileStream fs = new FileStream(path, FileMode.Open);
            byte[] sign = new byte[4];
            fs.Read(sign, 0, 4);
            uint sg = BitConverter.ToUInt32(sign, 0);
            //Console.WriteLine(BitConverter.ToString(sign, 0));
            if (sg != MODULE_SIGN)
            {
                throw new CorruptedModuleException(path, "Bad signature");
            }
            //fs.ReadByte();

            byte[] headers_end = new byte[4];
            fs.Read(headers_end, 0, 4);
            fs.Seek(4, SeekOrigin.Current); //sizeof(ModuleFlags) TODO: MAY FAIL!!!!!!!!!!!
            readHeaders(fs, BitConverter.ToInt32(headers_end, 0));
            readSegments(fs);
            fs.Close();
            return this;
        }

        private void readHeaders(FileStream fs, int end)
        {
            while (fs.Position != end)
            {
                SegmentHeader h = new SegmentHeader();
                char ch = (char)fs.ReadByte();
                while (ch != 0)
                {
                    h.Name += ch;
                    ch = (char)fs.ReadByte();
                }
                h.Type = (HeaderType)(byte)fs.ReadByte();
                byte[] offset = new byte[4];
                fs.Read(offset, 0, 4);
                h.StartPosition = BitConverter.ToUInt32(offset, 0);
                fs.Read(offset, 0, 4);
                h.EndPosition = BitConverter.ToUInt32(offset, 0);

                headers.Add(h);
            }
        }

        private void readSegments(FileStream fs)
        {
            foreach (var header in headers)
            {
                fs.Position = header.StartPosition;
                switch (header.Type)
                {
                    case HeaderType.Functions:
                    {
                        functions = new List<Function>();
                        int fcount = fs.ReadInt();

                        while (fs.Position != header.EndPosition)
                        {
                            Function mf = new Function("", new DataType(DataTypes.Null));
                            mf.module = this.name;
                            mf.type = (DataTypes)(byte)fs.ReadByte();
                            char ch = (char)fs.ReadByte();
                            while (ch != '\0')
                            {
                                mf.name += ch;
                                ch = (char)fs.ReadByte();
                            }
                            ch = (char)fs.ReadByte();
                            while (ch != '\0')
                            {
                                mf.argTypes.Add(new VarType((DataTypes)ch));
                                ch = (char)fs.ReadByte();
                            }
                            //mf.argTypes
                            mf.isPublic = !fs.ReadBool();

                            mf.flags = (byte)fs.ReadByte();
                            if ((mf.flags & Function.F_IMPORTED) != 0) //imported
                            {
                                string module = fs.ReadCStr();
                                if (!compiler.imported.Exists(m => m.path == module))
                                {
                                    Module mod = new Module(compiler);
                                    mod.Load(module);
                                    compiler.imported.Add(mod);
                                }
                            }
                            else if((mf.flags & Function.F_RTINTERNAL) == 0)
                            {
                                uint var_count = fs.ReadUInt();
                                if (var_count > 0)
                                {
                                    uint vs = fs.ReadUInt();
                                    fs.Position += var_count;
                                }
                                uint bs = fs.ReadUInt();
                                fs.Position += bs;
                            }
                            if(mf.isPublic)
                                functions.Add(mf);
                        }
                    }
                        break;
                    case HeaderType.Globals:
                        {
                            globals = new List<ModuleGlobal>();
                            int gcount = fs.ReadInt();

                            while (fs.Position != header.EndPosition)
                            {
                                ModuleGlobal mg = new ModuleGlobal();
                                mg.Type = (DataTypes)(byte)fs.ReadByte();
                                mg.isPrivate = fs.ReadBool();
                                mg.Name = fs.ReadCStr();
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

