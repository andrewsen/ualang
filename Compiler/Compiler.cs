//
//  Compiler.cs
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
using System.Linq;
using System.Text;
using System.Xml;
using CompilerClasses;
using Language;
using Compiler;
using System.Globalization;

namespace Translator
{
    public class Compiler
    {
        string code;
        string vasm;

        List<object> dirs = new List<object>();

        internal List<Function> funcs = new List<Function>();
        internal List<Module> imported = new List<Module>();
        internal List<Metadata> meta = new List<Metadata>();
        internal List<AttributeObject> bindedAttributes = new List<AttributeObject>();

        TokenStream tokens;
        CodeBlock globalVars = new CodeBlock();
        OpPriority priorities = new OpPriority();
        Function currentFun = null;
        int brace_counter = 0, for_label_counter = 0, while_label_counter = 0, do_while_label_counter = 0, if_label_counter = 0, switch_label_counter = 0;
        bool isPublic = false, isNative = false, noreturn = false;
        string[] _in;
        string _out;

        public static Compiler GetTestClass()
        {
            return new Compiler();
        }

        [Obsolete]
        public Compiler() // Demo
        {
        /*
            funcs.Add(new Function("fun", DataTypes.Null));
            funcs.Add(new Function("funA", DataTypes.Null));
            funcs.Add(new Function("funB", DataTypes.Null));
            funcs.Add(new Function("exp", DataTypes.Null));
            funcs.Add(new Function("func", DataTypes.Null));
            */
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Translator.Compiler"/> class.
        /// </summary>
        /// <param name="_in">In.</param>
        /// <param name="_out">Out.</param>
        public Compiler(string[] _in, string _out)
        {
            //imported.Add(Module.CreateVmInternal());
            /*imported.Add(new Function("::vm.internal", "print", DataTypes.Void, new List<VarType> {new VarType(DataTypes.String)}));
            imported.Add(new Function("::vm.internal", "print", DataTypes.Void, new List<VarType> {new VarType(DataTypes.Int)}));
            imported.Add(new Function("::vm.internal", "print", DataTypes.Void, new List<VarType> {new VarType(DataTypes.Double)}));
            imported.Add(new Function("::vm.internal", "reads", DataTypes.String, new List<VarType> {}));
            imported.Add(new Function("::vm.internal", "readi", DataTypes.Int, new List<VarType> {}));
            imported.Add(new Function("::vm.internal", "readd", DataTypes.Double, new List<VarType> {}));*/
            //funcs.Add(new Function("fun", DataTypes.Null));
            //funcs.Add(new Function("exp", DataTypes.Null));
            //funcs.Add(new Function("func", DataTypes.Null));

            priorities.Add("=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=");
            priorities.Add("||");
            priorities.Add("&&");
            priorities.Add("|");
            priorities.Add("^");
            priorities.Add("&");
            priorities.Add("==", "!=");
            priorities.Add(">", ">=", "<=", "<");
            priorities.Add(">>", "<<");
            priorities.Add("+", "-");
            priorities.Add("*", "/", "%");
            priorities.Add("#!", "#~", "#+", "#-", "#++", "#--");
            priorities.Add("++", "--", ".", "[]", "new");
            priorities.Add("cast");
            priorities.Add("");
            funcs.Add(new Function("__global_constructor__", new DataType(DataTypes.Void)));

            this._in = _in;
            this._out = _out;
            //tokens = new TokenStream(new StreamReader(_in).ReadToEnd());
        }

        public void Compile()
        {
            foreach (string f in _in)
            {
                tokens = new TokenStream(new StreamReader(f).ReadToEnd());
                CommandArgs.source = f;
                compileGlobal();
            }
            if (globalVars.Locals.Count == 0)
                funcs.RemoveAt(0);
            //Console.ReadKey();
            foreach (var fun in funcs)
            {
                if ((fun.flags & Function.F_RTINTERNAL) == 0)
                {
                    currentFun = fun;
                    fun.mainBlock = compileBlock(new TokenStream(ref fun.body), globalVars);
                }
            }
            foreach (var md in meta)
            {
                dirs.Add(md);
            }

            ILCompiler ilc = new ILCompiler(dirs, funcs, globalVars, _out);
            ilc.Compile();
            
            if (CommandArgs.writeTac)
                writeTac();
            //return;

            return;
        }

        private void writeTac()
        {
            StreamWriter sw = new StreamWriter(CommandArgs.outTacFile ?? (_out + ".tac"));

            sw.WriteLine("globals [");
            for (var i = 0; i < globalVars.Locals.Count; ++i)
            {
                var g = globalVars.Locals[i].Var;
                sw.Write("  " + VarType.FIELD_PREFIX + i + " : " + g.type.ToString() + " -> ");
                sw.Write((g.isPublic ? "public " : "private "));
                if (g.isPublic)
                    sw.WriteLine(g.name.Remove(0, VarType.FIELD_PREFIX.Length));
                else
                    sw.WriteLine("");
                //if (g.val != null)
                //  sw.WriteLine (" = " + g.val.ToString ());
                //else
                //  sw.WriteLine ("");
            }
            sw.WriteLine("]");
            sw.WriteLine("");

            int offset = 0;

            foreach (var f in funcs)
            {
                sw.Write((f.isPublic ? "public" : "private") + " " + f.type.ToString() + " " + f.name + "(");
                for (var i = 0; i < f.argTypes.Count; ++i)
                {
                    var v = f.argTypes[i];
                    sw.Write(v.type.ToString() + " " + VarType.ARG_PREFIX + i + " ");
                }
                sw.WriteLine(")");
                if (f.locals.Count != 0)
                {
                    sw.WriteLine("  locals [");
                    for (int i = 0; i < f.locals.Count; ++i)
                    {
                        var l = f.locals[i];
                        sw.WriteLine("    " + VarType.LOCAL_PREFIX + i + " : " + l.type.ToString());
                    }
                    sw.WriteLine("  ]");
                }
                writeBlock(f.mainBlock, sw, 0);
            }

            sw.Close();

        }

        private void writeBlock(CodeBlock codeBlock, StreamWriter sw, int offset)
        {
            sw.WriteLine(new String(' ', offset) + "{");
            offset += 4;
            foreach (object obj in codeBlock.Inner)
            {
                if (obj is CodeBlock)
                {
                    writeBlock((CodeBlock)obj, sw, offset);
                }
                else if (obj is If)
                {
                    If _if = obj as If;
                    for (int i = 0; i < _if.Cond.Inner.Count - 1; ++i)
                    {
                        sw.WriteLine(new String(' ', offset) + (_if.Cond.Inner[i] as BasicPrimitive).Expr);
                    }
                    sw.Write(new String(' ', offset) + "if " + (_if.Cond.Inner.Last() as BasicPrimitive).Expr);
                    sw.WriteLine(" " + _if.Label);

                    //if (_if.Body is CodeBlock)
                    writeBlock(_if.Body as CodeBlock, sw, offset);
                    //else 
                    //  sw.WriteLine (new String (' ', offset + 4) + (_if.Body as BasicPrimitive).Expr);
                    foreach (If elif in _if.ElseIfs)
                    {
                        for (int i = 0; i < elif.Cond.Inner.Count - 1; ++i)
                        {
                            sw.WriteLine(new String(' ', offset) + (elif.Cond.Inner[i] as BasicPrimitive).Expr);
                        }
                        sw.WriteLine(new String(' ', offset) + "else if " + (elif.Cond.Inner.Last() as BasicPrimitive).Expr);
                        //if (elif.Body is CodeBlock)
                        writeBlock(elif.Body as CodeBlock, sw, offset);
                        //else 
                        //  sw.WriteLine (new String (' ', offset + 4) + (elif.Body as BasicPrimitive).Expr);
                    }
                    //if (_if.Else != null) {
                    //  sw.WriteLine(new String (' ', offset) + "else ");
                    //  if (_if.Else is CodeBlock)
                    //      writeBlock (_if.Else as CodeBlock, sw, offset);
                    //  else 
                    //      sw.WriteLine (new String (' ', offset + 4) + (_if.Else as BasicPrimitive).Expr);
                    //}
                }
                else if (obj is Goto)
                {
                    Goto g = obj as Goto;
                    sw.WriteLine(new String(' ', offset) + "goto " + g.Label);
                }
                else if (obj is Label)
                {
                    Label l = obj as Label;
                    sw.WriteLine(l.StrLabel + ":");
                }
                else if (obj is While)
                {
                    While w = obj as While;
                    //sw.WriteLine (new String (' ', offset) + "while " + w.Cond.Expr);
                    if (w.Body is CodeBlock)
                        writeBlock(w.Body as CodeBlock, sw, offset);
                    else
                        sw.WriteLine(new String(' ', offset + 4) + (w.Body as BasicPrimitive).Expr);
                }
                else if (obj is Switch)
                {
                    Switch swt = obj as Switch;
                    writeBlock(swt.Cond as CodeBlock, sw, offset + 4);
                    writeBlock(swt.Jumps as CodeBlock, sw, offset + 4);
                    writeBlock(swt.Body as CodeBlock, sw, offset + 4);
                }
                else if (obj is Return)
                {
                    Return r = obj as Return;
                    sw.WriteLine(new String(' ', offset) + "return " + (r.noData ? "" : r.Statement.Expr));
                }
                else if (obj is For)
                {
                    For f = obj as For;
                    writeBlock(f.forBlock, sw, offset);
                }
                else if (obj is BasicPrimitive)
                {
                    BasicPrimitive bp = obj as BasicPrimitive;
                    if (bp.Polish.Count == 1 && bp.Polish[0].Type != TokType.Function)
                        continue;//bp.Expr = "//" + bp.Expr;
                    sw.WriteLine(new String(' ', offset) + bp.Expr);
                }
            }
            sw.WriteLine(new String(' ', offset - 4) + "}");
            //o.Remove(o.Length - 4);
        }

        public void compileGlobal()
        {
            funcs[0].body = "{";
            while (tokens.Type != TokenType.EOF)
            {
                tokens.Next();

                if (CommandArgs.lang == SyntaxLang.English)
                    switch (tokens.ToString())
                    {
                        case "public":
                            if (isPublic)
                                throw new CompilerException(ExceptionType.ExcessToken, "Excess modifier 'public'", tokens);
                            isPublic = true;
                            break;
                        case "private":
                            isPublic = false;
                            break;
                        case "native":
                            if (isNative)
                                throw new CompilerException(ExceptionType.ExcessToken, "Excess modifier 'native'", tokens);
                            isNative = true;
                            break;
                        /*case "struct":
                            evalStruct();
                            break;
                        case "@":
                            evalAttribute();
                            break;*/
                        case "import":
                            evalImport();
                            break;
                        case "void":
                            evalDeclaration(DataTypes.Void);
                            break;
                        case "byte":
                            evalDeclaration(DataTypes.Byte);
                            break;
                        case "short":
                            evalDeclaration(DataTypes.Short);
                            break;
                        case "int":
                            evalDeclaration(DataTypes.Int);
                            break;
                        case "uint":
                            evalDeclaration(DataTypes.Uint);
                            break;
                        case "long":
                            evalDeclaration(DataTypes.Long);
                            break;
                        case "ulong":
                            evalDeclaration(DataTypes.Ulong);
                            break;
                        case "double":
                            evalDeclaration(DataTypes.Double);
                            break;
                        case "bool":
                            evalDeclaration(DataTypes.Bool);
                            break;
                        case "string":
                            evalDeclaration(DataTypes.String);
                            break;
                        case "@":
                            evalAttribute();
                            break;
                        default:
                            if (tokens.Type == TokenType.Identifier)
                                evalDeclaration(DataTypes.User, tokens.ToString());
                            break;
                    }
                else
                    switch (tokens.ToString())
                    {
                        case "публічна":
                        case "публічний":
                            if (isPublic)
                                throw new CompilerException(ExceptionType.ExcessToken, "Excess modifier 'public'", tokens);
                            isPublic = true;
                            break;
                        case "приватна":
                        case "приватний":
                            isPublic = false;
                            break;
                        case "нативна":
                        case "нативний":
                            if (isNative)
                                throw new CompilerException(ExceptionType.ExcessToken, "Excess modifier 'native'", tokens);
                            isNative = true;
                            break;
                    /*case "struct":
                            evalStruct();
                            break;
                        case "@":
                            evalAttribute();
                            break;*/
                        case "імпорт":
                            evalImport();
                            break;
                        case "воід":
                            evalDeclaration(DataTypes.Void);
                            break;
                        case "байт":
                            evalDeclaration(DataTypes.Byte);
                            break;
                        case "коротке":
                            evalDeclaration(DataTypes.Short);
                            break;
                        case "зціле":
                            evalDeclaration(DataTypes.Int);
                            break;
                        case "ціле":
                            evalDeclaration(DataTypes.Uint);
                            break;
                        case "здовге":
                            evalDeclaration(DataTypes.Long);
                            break;
                        case "довге":
                            evalDeclaration(DataTypes.Ulong);
                            break;
                        case "подвійне":
                            evalDeclaration(DataTypes.Double);
                            break;
                        case "буль":
                            evalDeclaration(DataTypes.Bool);
                            break;
                        case "рядок":
                            evalDeclaration(DataTypes.String);
                            break;
                        case "@":
                            evalAttribute();
                            break;
                        default:
                            if (tokens.Type == TokenType.Identifier)
                                evalDeclaration(DataTypes.User, tokens.ToString());
                            break;
                    }

            }
            funcs[0].body += localize("return") + ";\n";
            funcs[0].body += "}";

        }

        internal CodeBlock compileBlock(TokenStream toks, CodeBlock parent, bool isCurrent = false)
        {
            /*
             * {
             *     ...
             *     {
             *         ...
             *     }
             *     ...
             * }
             */
            //Token toks = new Token(ref code);
            //f.mainBlock = new CodeBlock(globalVars);
            CodeBlock blk;
            if (isCurrent)
                blk = parent;
            else
                blk = new CodeBlock(parent);
            if (toks.Next() != "{")
                throw new CompilerException(ExceptionType.Brace, "Block curly brace '{' expected", toks);
            toks.Next();

            while (toks.Type != TokenType.EOF)
            {
                if (toks.ToString() == ";" && toks.Type == TokenType.Separator)
                {
                    toks.Next();
                    continue;
                }
                if (CommandArgs.lang == SyntaxLang.English)
                    switch (toks.ToString())
                    {
                        case "break":
                            evalBreak(toks, blk);
                            break;
                        case "continue":
                            evalContinue(toks, blk);
                            break;
                        case "for":
                            evalFor(toks, blk);
                            break;
                        case "if":
                            evalIf(toks, blk);
                            break;
                        case "switch":
                            evalSwitch(toks, blk);
                            break;
                        case "while":
                            evalWhile(toks, blk);
                            break;
                        case "return":
                            evalReturn(toks, blk);
                            break;
                        case "goto":
                            evalGoto(toks, blk);
                            break;
                        case "do":
                            evalDoWhile(toks, blk);
                            break;
                        case "{":
                            toks.PushBack();
                            blk.Inner.Add(evalBlock(toks, blk));
                            toks.Next();
                            break;
                        case "}":
                            return blk;
                        default:
                            if (isType(toks.ToString()))
                            {
                                //toks.PushBack();
                                evalLocalVar(toks.ToString(), toks, blk);
                            }
                            else
                                evalExpr(toks, blk);
                            break;
                    }
                else
                    switch (toks.ToString())
                    {
                        case "зупинити":
                            evalBreak(toks, blk);
                            break;
                        case "продовжити":
                            evalContinue(toks, blk);
                            break;
                        case "для":
                            evalFor(toks, blk);
                            break;
                        case "якщо":
                            evalIf(toks, blk);
                            break;
                        case "поки":
                            evalWhile(toks, blk);
                            break;
                        case "повернути":
                            evalReturn(toks, blk);
                            break;
                        case "перейти":
                            evalGoto(toks, blk);
                            break;
                        case "робти":
                            evalDoWhile(toks, blk);
                            break;
                        case "{":
                            toks.PushBack();
                            blk.Inner.Add(evalBlock(toks, blk));
                            toks.Next();
                            break;
                        case "}":
                            return blk;
                        default:
                            if (isType(toks.ToString()))
                            {
                                //toks.PushBack();
                                evalLocalVar(toks.ToString(), toks, blk);
                            }
                            else
                                evalExpr(toks, blk);
                            break;
                    }
            }
            return null;
        }

        private void evalExpr(TokenStream toks, CodeBlock blk)
        {
            string expr = "", id = toks.ToString();
            toks.PushBack();
            int pos = toks.Position;
            toks.Next();
            if (toks.Type == TokenType.Identifier && toks.Next() == ":")
            {
                toks.PushBack();
                evalLabel(toks, blk);
                return;
            }
            else
                toks.Position = pos;
            while (toks.Next() != ";" && toks.Type != TokenType.Separator)
            {
                expr += toks.ToString() + " ";
            }
            blk.Inner.Add(evalExpr(expr, blk));
        }

        /// <summary>
        /// Evals the expr.
        /// </summary>
        /// <returns>The expr.</returns>
        /// <param name="str">String.</param>
        /// <param name="blk">Blk.</param>
        internal BasicPrimitive evalExpr(string str, CodeBlock blk)
        {
            // i = arr[5][1]
            // i arr 5 [] 1 [] = 

            // i = arr [x+4] * 8
            // i arr x 4 + [] 8 * = 
            // i arr x+4 [] 8 * =
            // i arr[x+4] 8 * =

            // i = a1[a2[42]]
            // i a1 a2 42 [] [] =
            // i a1 a2[42] [] =

            // arr [i+1][0][1] | int new +  =

            //t = new Class();
            //t Class() new = 

            // i = 1 + exp(x * 2 - 1) + (x - 3)
            // i 1 x 2 * 1 - exp + x 3 - + =

            // i 1 x 2 * 1 - exp + x 3 - + =
            // i 1 [2x] 1 - exp + x 3 - + =
            // i 1 [2x-1] exp + x 3 - + =
            // i 1 [exp(2x-1)] + x 3 - + =
            // i [1+exp(2x-1)] x 3 - + =
            // i [1+exp(2x-1)] [x-3] + =
            // i [1+exp(2x-1) + (x-3)] =
            // i = 1+exp(2x-1) + (x-3)

            // i 1 7 exp + x 3 - + =

            var stack = new Stack<Token>();
            TokenStream infix = new TokenStream(ref str);

            var polish = new List<Token>();

            var newStack = new Stack<Token>();
            
            infix.Next();

            bool isUnary = true, isBracket = false, isNewArr = false;
            int fcount = 0;
            while (infix.Type != TokenType.EOF)
            {
                if (infix.Type == TokenType.Digit) // TODO: add other types (ui8, i16...) 
                {
                    string digit = infix.ToString();
                    if (isUnary && stack.Count > 0 && stack.Peek().Type == TokType.Operator && stack.Peek().StringRep == "#-")
                    {
                        digit = "-" + digit;
                        stack.Pop();
                    }
                    if (digit.Contains('.'))
                        polish.Add(new Token(digit, TokType.Double, DataTypes.Double));
                    else
                        polish.Add(new Token(digit, TokType.Int, DataTypes.Int));
                    isUnary = false;
                }
                else if (isBracket && isType(infix.ToString()))
                {
                    stack.Pop();
                    var type = infix.ToString();
                    if (infix.Next() != ")")
                        throw new CompilerException(ExceptionType.Brace, ") expected", infix);

                    while (stack.Count != 0 && checkTopOp("cast", stack.Peek(), isUnary))
                    {
                        polish.Add(stack.Pop());
                    }
                    stack.Push(new Token(type, TokType.OperatorCast));
                    isUnary = true;

                }
                else if (infix.Type == TokenType.Identifier && infix.ToString() == "new")
                {
                    var type = infix.Next();
                    if (!isType(type))
                        throw new CompilerException(ExceptionType.IllegalType, "valid type expected after `new`", infix);

                    var tok = new Token("new[]", TokType.OperatorNew, getPODType(type));

                    if (infix.Next() != "[")
                    {
                        infix.PushBack();
                        throw new NotImplementedException();
                    }
                    stack.Push(new Token(infix.ToString(), TokType.OpenBrc));
                    isUnary = isBracket = true;

                    tok.NewArrayDimens = 1;

                    TokenStream ts = new TokenStream(infix.Source);
                    ts.Position = infix.Position;

                    int bc = 1;
                    //bool ob = true;

                    ts.Next();
                    while (bc >= 0)
                    {
                        if (ts.ToString() == "[")
                            ++bc;
                        else if (ts.ToString() == "]")
                        {
                            --bc;
                            if(bc == 0)
                                break;
                        }
                        ts.Next();
                    }
                    //ts.Source[ts.Position] = ")";     
                    char[] charr = ts.Source.ToCharArray();
                    charr[ts.Position-1] = ')';
                    //infix.Source = new string(charr);

                    List<int> poss = new List<int>();
                    while(true)
                    {
                        if (ts.Next() != "[")
                        {
                            ts.PushBack();
                            break;
                        }
                        else
                            poss.Add(ts.Position-1);
                            //ts.Source[ts.Position] = " ";

                        tok.NewArrayDimens++;
                        if(ts.Next() != "]")
                            throw new CompilerException(ExceptionType.Brace, "`]` expected", ts);
                        else
                            poss.Add(ts.Position-1);
                            //ts.Source[ts.Position] = " ";
                        
                    }

                    //char[] charr = ts.Source.ToCharArray();
                    //charr[ts.Position] = ')';
                    foreach(int p in poss)
                    {
                        charr[p] = ' ';
                    }

                    infix.Source = new string(charr);
                    //newStack.Push(tok);

                    stack.Push(tok);
                    //continue;
                }
                else if(infix.ToString() == "]")
                {
                    var pop = stack.Pop();
                    while (pop.Type != TokType.ArrayElemOp)
                    {
                        if (stack.Count == 0)
                            throw new CompilerException(ExceptionType.BadExpression, "Bad expression: " + str, null);
                        polish.Add(pop);
                        pop = stack.Pop();
                    }
                    //if (stack.Count == 0)

                    if (stack.Count != 0 && stack.Peek().Type == TokType.Function)
                        polish.Add(stack.Pop());

                    polish.Add(new Token("[]", TokType.ArrayElemOp));
                    isUnary = false;
                }

                else if(infix.ToString() == "[")
                {
                    if(!new TokType[] {TokType.ArrayElemOp, TokType.Function, TokType.Variable, TokType.String}.Contains(polish.Last().Type))
                        throw new CompilerException(ExceptionType.IllegalType, "Variable or function expected", null);
                    stack.Push(new Token("[", TokType.ArrayElemOp));
                    isUnary = isBracket = true;
                    infix.Next();
                    continue;
                }

                else if (infix.Type == TokenType.Identifier)
                {
                    bool isf = false;
                    Function func = null;
                    //if(
                    if (funcs.Any(f => f.name == infix.ToString()) || imported.Any(m => m.functions.Any(f => f.name == infix.ToString())))
                    {
                        isf = true;
                        //func = f;
                        ++fcount;
                    }
                    //else if(); //TODO: Modules are near here...
                    if (isf)
                    {
                        //"1" - 1+1
                        //1, 2, 4, 2, 3, 4, 2, 3, 4, 0
                        //i = 3 + temp += v
                        // i = 3 + temp = temp + v
                        TokenStream ts = new TokenStream(infix.Source);
                        ts.Position = infix.Position;

                        if (ts.Next() != "(")
                            throw new CompilerException(ExceptionType.Brace, "Open Brace after function call expected",
                                ts);
                        int bc = 0, cc = 0;
                        bool ob = true;

                        ts.Next();
                        while (bc >= 0)
                        {
                            if (ts.ToString() == "(")
                                ++bc;
                            else if (ts.ToString() == ")")
                            {
                                --bc;
                                if (ob)
                                    cc = -1;
                            }
                            else if (bc == 0 && ts.ToString() == ",")
                                cc++;

                            ob = false;
                            ts.Next();
                        }


                        string newname = infix.ToString() + "@";
                        newname += (cc + 1).ToString();

                        stack.Push(new Token(newname, TokType.Function));
                    }
                    else
                    {
                        var varName = infix.ToString();
                        DataTypes varType = DataTypes.Null;
                        CodeBlock b = blk.Parent;
                        int level = 0;

                        Token tok = new Token(varName, TokType.Variable);

                        while (b != null)
                        {
                            ++level;
                            b = b.Parent;
                        }
                        b = blk;
                        if (!varName.StartsWith(VarType.LOCAL_PREFIX) && !varName.StartsWith(VarType.ARG_PREFIX))
                        {
                            bool isArg = false;
                            int idx = 0;
                            while (true)
                            {
                                foreach (var vd in b.Locals)
                                {
                                    if (vd.Var.name == VarType.LOCAL_PREFIX + varName)
                                    {
                                        idx = vd.LocalIndex;
                                        tok.DType = vd.Var.type;
                                        tok.NewArrayDimens = vd.Var.arrayDimens;
                                        tok.BaseArrayType = vd.Var.nested.type;
                                        goto outer;
                                    }
                                }
                                --level;
                                b = b.Parent;
                                if (b == null)
                                    break;
                            }
                            outer:
                            if (level == -1)
                            {
                                for (int i = 0; i < currentFun.argTypes.Count; ++i)
                                {
                                    if (currentFun.argTypes[i].name == VarType.ARG_PREFIX + varName)
                                    {
                                        isArg = true;
                                        level = 1;
                                        idx = i;
                                        tok.DType = currentFun.argTypes[i].type;
                                        tok.NewArrayDimens = currentFun.argTypes[i].arrayDimens;
                                        tok.BaseArrayType = currentFun.argTypes[i].nested.type;
                                    }
                                }
                                for (int i = 0; i < globalVars.Locals.Count; ++i)
                                {
                                    if (globalVars.Locals[i].Var.name == VarType.FIELD_PREFIX + varName)
                                    {
                                        level = 0;
                                        idx = i;
                                        tok.DType = globalVars.Locals[i].Var.type;
                                        tok.NewArrayDimens = globalVars.Locals[i].Var.arrayDimens;
                                        tok.BaseArrayType = globalVars.Locals[i].Var.nested.type;
                                    }
                                }
                                if (level == -1)
                                    throw new Exception("Unknown var " + varName);
                            }

                            if (isArg)
                                tok.StringRep = VarType.ARG_PREFIX + idx;
                            else if (level == 0)
                                tok.StringRep = VarType.FIELD_PREFIX + idx;
                            else
                                tok.StringRep = VarType.LOCAL_PREFIX + idx;
                        }
                        //var tok = new Token(varName, TokType.Variable, varType);
                        //tok.
                        polish.Add(tok);
                    }
                    isUnary = false;
                }
                else if (infix.ToString() == "(")
                {
                    stack.Push(new Token(infix.ToString(), TokType.OpenBrc));
                    isUnary = isBracket = true;
                    infix.Next();
                    continue;
                }
                else if (infix.ToString() == ",")
                {
                    while (stack.Count != 0 && stack.Peek().Type != TokType.OpenBrc)
                    {
                        polish.Add(stack.Pop());
                    }
                    isUnary = true;
                }
                else if (infix.ToString() == ")")
                {
                    var pop = stack.Pop();
                    while (pop.Type != TokType.OpenBrc)
                    {
                        if (stack.Count == 0)
                            throw new CompilerException(ExceptionType.BadExpression, "Bad expression: " + str, null);
                        polish.Add(pop);
                        pop = stack.Pop();
                    }
                    //if (stack.Count == 0)

                    if (stack.Count != 0 && stack.Peek().Type == TokType.Function)
                        polish.Add(stack.Pop());
                    isUnary = false;
                }
                else if (infix.Type == TokenType.Operator || infix.Type == TokenType.OperatorAssign)
                {
                    while (stack.Count != 0 && checkTopOp(infix.ToString(), stack.Peek(), isUnary))
                    {
                        polish.Add(stack.Pop());
                    }
                    if (isUnary)
                        stack.Push(new Token("#" + infix.ToString(), TokType.Operator));
                    else
                        stack.Push(new Token(infix.ToString(), TokType.Operator));
                    isUnary = true;
                }
                else if (infix.Type == TokenType.Boolean)
                {
                    polish.Add(new Token(infix.ToString(), TokType.Boolean, DataTypes.Bool));
                    isUnary = false;
                }
                else if (infix.Type == TokenType.String)
                {
                    polish.Add(new Token(infix.ToString(), TokType.String, DataTypes.String));
                    isUnary = false;
                }
                isBracket = false;
                infix.Next();
            }

            while (stack.Count != 0)
                polish.Add(stack.Pop());
            
            string res = polish.Aggregate("", (current, p) => current + (p.StringRep + " "));
            var bp = new BasicPrimitive(DataTypes.Void, res.Trim(), polish);
            //return bp;
            //Console.WriteLine("RPN: " + res.Trim());
            var tac = CommandArgs.newTac ? evalTACNew(bp) : evalTAC(bp);
            for (int i = 0; i < tac.Count - 1; ++i)
            {
                tacSetType(tac[i]);
                blk.Inner.Add(tac[i]);
            }
            return tac.Last();
        }

        /// <summary>
        /// Evals the TAC new.
        /// </summary>
        /// <returns>The TAC new.</returns>
        /// <param name="bp">Bp.</param>
        List<BasicPrimitive> evalTACNew(BasicPrimitive bp)
        {
            List<BasicPrimitive> tac = new List<BasicPrimitive>();

            if (bp.Polish.Count == 1 && bp.Polish[0].Type != TokType.TempVar && bp.Polish[0].Type != TokType.Function)
            {
                var tbp = new BasicPrimitive();
                tbp.Polish.Add(bp.Polish[0]);
                tbp.Expr = bp.Polish[0].StringRep;
                tbp.mainOpIdx = 0;
                tac.Add(tbp);
                return tac;
            }

            var rpn = bp.Polish;
            //int start_count = rpn.Count;

            Stack<Token> ts = new Stack<Token>();
            //int idx = 0;
            while (rpn.Count > 0)
            {
                bool isop = false;
                var token = rpn.First();

                var testTok = new TokType [] { TokType.Operator, TokType.OperatorAssign, TokType.Function, TokType.OperatorCast, TokType.OperatorNew, TokType.ArrayElemOp };

                if (testTok.Contains(token.Type))
                {
                    isop = true;
                    var op = token.StringRep;

                    var testOp1 = new string [] { "==", "!=", "<", "<=", ">=", ">" };
                    var testOp2 = new string[] { "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=" };
                    if (op.StartsWith("#") || op == "++" || op == "--")
                        tacEvalUnaryOp(token, ts, tac);
                    else if (testOp1.Contains(op))
                        tacEvalCmpOp(token, ts, tac);
                    else if (op == "||" || op == "&&")
                        tacEvalLogicOp(token, ts, tac);
                    else if (testOp2.Contains(op))
                        tacEvalShortAssign(token, ts, tac);
                    else if (op == "=")
                        tacEvalAssignOp(token, ts, tac);
                    else if (token.Type == TokType.OperatorCast)
                        tacEvalCast(token, ts, tac);
                    else if (token.Type == TokType.OperatorNew)
                        tacEvalNew(token, ts, tac);
                    else if (token.Type == TokType.ArrayElemOp)
                        tacEvalArrElem(token, ts, tac);
                    else if (token.Type == TokType.Function)
                        tacEvalFun(token, ts, tac, rpn);
                    else
                        tacEvalArithmOp(token, ts, tac);
                    //rpn.RemoveAt(0);
                }
                else
                {
                    BasicPrimitive tbp = new BasicPrimitive();
                    var tmp = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, token.DType);
                    if(token.DType == DataTypes.Array)
                    {
                        tmp.BaseArrayType = token.BaseArrayType;
                        tmp.NewArrayDimens = token.NewArrayDimens;   
                    }

                    tmp.TempRef = token;

                    tbp.Expr = tmp.StringRep + " = " + token.StringRep;
                    tbp.Polish.Add(tmp);
                    tbp.Polish.Add(new Token("=", TokType.OperatorAssign));
                    tbp.Polish.Add(token);
                    tbp.mainOpIdx = 1;

                    ++currentFun.tempVarCounter;

                    tac.Add(tbp);

                    ts.Push(tmp);
                    rpn.RemoveAt(0);
                }

                if (isop)
                {
                    rpn.Remove(token);
                }
            }

            //if(start_count != 1)
            if (ts.Count != 0)
            {
                var tbp = new BasicPrimitive();
                var tok = ts.Pop();
                tbp.Polish.Add(tok);
                tbp.Expr = tok.StringRep;
                tbp.mainOpIdx = 0;
                tac.Add(tbp);
            }
            while (ts.Count != 0)
            {
                var tok = ts.Pop();
                foreach (var prim in tac.ToList())
                {
                    if (prim.Polish.First().StringRep == tok.StringRep)
                    {
                        var lst = from t in tac
                            where t.Polish.First().StringRep != tok.StringRep && t.Polish.Contains(tok)
                                select t;
                        if (lst.ToList().Count == 0)
                            tac.Remove(prim);
                    }
                }
                return tac.ToList();
            }

            return tac;
        }

        void tacEvalUnaryOp(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {
            var op = tok.StringRep;
            var taccode = new BasicPrimitive();
            Token tmp;
            var arg = ts.Pop();
            if (op == "#!")
            {
                tmp = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, DataTypes.Bool);
                ts.Push(tmp);
                taccode.Polish.Add(tmp);
                taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                taccode.Polish.Add(new Token(op, TokType.OperatorMono));
                taccode.mainOpIdx = 2;


                if (getVarType(arg) == DataTypes.Bool)
                {
                    taccode.Expr = tmp.StringRep + " = " + op + " " + arg.StringRep;
                    //taccode.Type = DataTypes.Bool;
                    taccode.Polish.Add(arg);
                    ++currentFun.tempVarCounter;
                }
                else
                {
                    var conv = new BasicPrimitive();
                    var temp = convert(ref conv, arg, DataTypes.Bool);
                    tac.Add(conv);
                    taccode.Polish.Add(temp);
                    taccode.Expr = tmp.StringRep + " = " + op + " " + temp.StringRep;
                    //rpn[i - 1] = temp;
                }
                tac.Add(taccode);
            }
            /*else if (op == "++" || op == "--")
            {
                /*
                    t0 = loc0
                    t1 = t0
                    t2 = ++ t0
                    loc0 = t2
                    print (int) t1
                 * /
                if (!isNumeric(getVarType(arg)))
                    throw new CompilerException(ExceptionType.NonNumericValue, "Numeric value expected!", null);

                tmp = new Token ("t"+currentFun.tempVarCounter, TokType.TempVar, getVarType(arg));
                ts.Push(arg);
                ts.Push(arg);
                ts.Push(tmp);
                taccode.Expr = tmp.StringRep + " = " + op + " " + arg.StringRep;
                //taccode.Type = getVarType(rpn[i - 1]);
                taccode.Polish.Add(tmp);
                taccode.Polish.Add(new Token ("=", TokType.OperatorAssign));
                taccode.Polish.Add(new Token (op, TokType.OperatorMono));
                taccode.Polish.Add(arg);
                taccode.mainOpIdx = 2;
                ++currentFun.tempVarCounter;

                tac.Add(taccode);
                tacEvalAssignOp(tok, ts, tac);
                ts.Pop();
            }*/
            else if (op == "#++" || op == "#--" || op == "++" || op == "--")
                {
                    /*
                    t0 = loc0
                    t1 = t0
                    t2 = ++ t0
                    loc0 = t2
                    print (int) t1
                 */
                    if (!isNumeric(getVarType(arg)))
                        throw new CompilerException(ExceptionType.NonNumericValue, "Numeric value expected!", null);

                    tmp = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, getVarType(arg));
                    ts.Push(arg);
                    ts.Push(tmp);
                    taccode.Expr = tmp.StringRep + " = " + op + " " + arg.StringRep;
                    //taccode.Type = getVarType(rpn[i - 1]);
                    taccode.Polish.Add(tmp);
                    taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                    taccode.Polish.Add(new Token(op, TokType.OperatorMono));
                    taccode.Polish.Add(arg);
                    taccode.mainOpIdx = 2;
                    ++currentFun.tempVarCounter;

                    tac.Add(taccode);
                    tacEvalAssignOp(tok, ts, tac);
                }
                else
                {
                    /*
                    t0 = loc0
                    t1 = t0
                    t2 = ++ t0
                    loc0 = t2
                    print (int) t1
                 */
                    if (!isNumeric(getVarType(arg)))
                        throw new CompilerException(ExceptionType.NonNumericValue, "Numeric value expected!", null);

                    tmp = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, getVarType(arg));
                    ts.Push(arg);
                    ts.Push(tmp);
                    taccode.Expr = tmp.StringRep + " = " + op + " " + arg.StringRep;
                    //taccode.Type = getVarType(rpn[i - 1]);
                    taccode.Polish.Add(tmp);
                    taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                    taccode.Polish.Add(new Token(op, TokType.OperatorMono));
                    taccode.Polish.Add(arg);
                    taccode.mainOpIdx = 2;
                    ++currentFun.tempVarCounter;

                    tac.Add(taccode);
                    tacEvalAssignOp(tok, ts, tac);
                }
        }

        void tacEvalCmpOp(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {
            var res = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, DataTypes.Bool);

            var op = tok.StringRep;
            var taccode = new BasicPrimitive();

            taccode.Polish.Add(res);
            taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
            taccode.Expr = "t" + currentFun.tempVarCounter + " = ";
            ++currentFun.tempVarCounter;
            var t2 = ts.Pop();
            var t1 = ts.Pop();
            if (t1.DType == DataTypes.Void || t2.DType == DataTypes.Void)
                throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString() + " or " + t2.DType.ToString(), null);

            var conv = new BasicPrimitive();
            if (op == "==" || op == "!=")
            {
                if (t1.DType != t2.DType)
                {
                    //var conv = new BasicPrimitive();
                    if (t1.DType > t2.DType) //TEST
                    {
                        t2 = convert(ref conv, t2, t1.DType);
                        tac.Add(conv);
                        //rpn[i - 2] = t2;
                    }
                    else
                    {
                        t1 = convert(ref conv, t1, t2.DType);
                        tac.Insert(tac.Count - 1, conv); //TEST
                        //rpn[i - 1] = t1;
                    }
                    //tac.Add(conv);
                }

            }
            else if (!isNumeric(t1.DType) || !isNumeric(t2.DType))
                    throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString() + " or " + t2.DType.ToString(), null);
                else if (t1.DType > t2.DType)
                    {
                        t2 = convert(ref conv, t2, t1.DType);
                        //rpn[i - 2] = t2;
                        tac.Add(conv); //TEST
                    }
                    else if (t1.DType < t2.DType)
                        {
                            t1 = convert(ref conv, t1, t2.DType);
                            //rpn[i - 1] = t1;
                            tac.Insert(tac.Count - 1, conv); //TEST
                        }
            taccode.Polish.Add(t2);
            taccode.Polish.Add(new Token(op, TokType.Operator));
            taccode.Polish.Add(t1);
            taccode.mainOpIdx = 3;
            taccode.Expr += t2.StringRep + " " + op + " " + t1.StringRep;
            ts.Push(res);
            tac.Add(taccode);
            //rpn[i - 2] = res;
            //rpn.RemoveRange(i - 1, 2);
        }

        void tacEvalLogicOp(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {
            var res = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, DataTypes.Bool);

            var op = tok.StringRep;
            var taccode = new BasicPrimitive();

            taccode.Polish.Add(res);
            taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
            taccode.Expr = "t" + currentFun.tempVarCounter + " = ";
            ++currentFun.tempVarCounter;

            var t2 = ts.Pop();
            var t1 = ts.Pop();

            if (isNumeric(getVarType(t2)))
            {
                var conv = new BasicPrimitive();
                var temp = convert(ref conv, t2, DataTypes.Bool);
                tac.Insert(tac.Count - 1, conv); //FIXME: TEST TEST TEST
                taccode.Polish.Add(temp);
                taccode.Expr += temp.StringRep;
                //rpn[i - 2] = temp;
                //taccode.Expr += temp.StringRep;
            }
            else if (getVarType(t2) == DataTypes.Bool)
                {
                    taccode.Polish.Add(t2);
                    taccode.Expr += t2.StringRep;
                }
                else
                    throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t2.DType.ToString(), null);
            taccode.Polish.Add(new Token(op, TokType.Operator));
            taccode.Expr += " " + op + " ";
            if (isNumeric(getVarType(t1)))
            {
                var conv = new BasicPrimitive();
                var tmp = convert(ref conv, t1, DataTypes.Bool);
                tac.Add(conv); //TEST TEST TEST TEST
                taccode.Polish.Add(tmp);
                taccode.Expr += tmp.StringRep;
                //rpn[i - 1] = tmp;
                //taccode.Expr += tmp.StringRep;
            }
            else if (getVarType(t1) == DataTypes.Bool)
                {
                    taccode.Polish.Add(t1);
                    taccode.Expr += t1.StringRep;
                }
                else
                    throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString(), null);
            //rpn[i - 2] = res;
            //rpn.RemoveRange(i - 1, 2);
            taccode.mainOpIdx = 3;
            ts.Push(res);
            tac.Add(taccode);
        }

        void tacEvalAssignOp(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {   
            var taccode = new BasicPrimitive();

            Token t1 = ts.Pop(), t2 = ts.Pop();
            if (t2.TempRef.Type == TokType.Variable)
            {
                //tt = t2;
                ts.Push(t2);
                t2 = t2.TempRef;
            }
            //else if (t2.TempRef.Type == TokType.ArrayElem)
            //{
                // a =[] t1 t2 t3 | 
            //}

            uint dt1 = (uint)t1.DType, dt2 = (uint)t2.DType;

            //dt1 = 3 - dt2 = 3;

            if (t1.DType == DataTypes.Array && t2.DType == DataTypes.Array)
            {
                if(t1.NewArrayDimens != t2.NewArrayDimens)
                    throw new CompilerException(ExceptionType.IllegalType, "can't assign array of " 
                    + t1.NewArrayDimens + " dimens with array of " + t2.NewArrayDimens + " dimens", null);
                else if(t1.BaseArrayType != t2.BaseArrayType)
                        throw new CompilerException(ExceptionType.IllegalType, "can't assign of type " 
                        + t1.BaseArrayType.ToString() + " with array of type" + t2.BaseArrayType.ToString(), null);
            }
            else if (dt1 != dt2 && canCastType(t1.DType, t2.DType))
            {
                TokType tt1, tt2;
                if (t1.TempRef != null)
                    tt1 = t1.TempRef.Type;
                else
                    tt1 = t1.Type;
                if (t2.TempRef != null)
                    tt2 = t2.TempRef.Type;
                else
                    tt2 = t2.Type;
                if (tt1 == TokType.Double || tt1 == TokType.Variable
                    || tt2 == TokType.Double || tt2 == TokType.Variable)
                {
                    var conv = new BasicPrimitive();
                    t1 = convert(ref conv, t1, t2.DType);
                    //t1 = t1;
                    //if (dt1 > dt2)
                    tac.Add(conv);
                }
                else
                {
                    t1.DType = t2.DType;
                    if (t1.TempRef != null)
                        t1.TempRef.DType = t1.DType;
                }
                //    tac.Insert(tac.Count - 1, conv); //TEST
            }
            else if (dt1 != dt2)
                    throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString() + " or " + t2.DType.ToString(), null);

            taccode.Expr = t2.StringRep + " = " + t1.StringRep;
            taccode.Polish.Add(t2);
            taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
            taccode.Polish.Add(t1);
            taccode.mainOpIdx = 1;
            //if(tt != null)
            //    t2 = tt;
            ts.Push(t2);
            tac.Add(taccode);
        }

        void tacEvalShortAssign(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {   
            //t1 += t2
            //t3 = t1 + t2
            //t2 = t3
            //var op = tok.StringRep;
            var taccode = new BasicPrimitive();

            Token t1 = ts.Pop(), t2 = ts.Pop();
            uint dt1 = (uint)t1.DType, dt2 = (uint)t2.DType;

            //dt1 = 3 - dt2 = 3;

            if (dt1 != dt2 && canCastType(t1.DType, t2.DType))
            {
                var conv = new BasicPrimitive();
                t1 = convert(ref conv, t1, t2.DType);
                //t1 = t1;
                if (dt1 > dt2)
                    tac.Add(conv);
                else
                    tac.Insert(tac.Count - 1, conv); //TEST
            }
            else if (dt1 != dt2)
                    throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString() + " or " + t2.DType.ToString(), null);

            Token newOp = new Token(tok.StringRep.Replace("=", ""), TokType.Operator);

            //t2 += t1;
            //t3 = t2 + t1;
            //t2 = t3;

            ts.Push(t2);
            ts.Push(t1);
            ts.Push(t2);

            var testOp = new string[] { "+", "-", "*", "/" };
            if (testOp.Contains(newOp.StringRep))
                tacEvalArithmOp(newOp, ts, tac);

            tacEvalAssignOp(null, ts, tac);
        }

        void tacEvalArithmOp(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {
            var op = tok.StringRep;
            var taccode = new BasicPrimitive();

            Token t2 = ts.Pop(), t1 = ts.Pop();

            if(t2.TempRef != null && t1.TempRef != null && (t2.TempRef.Type == TokType.Double || t2.TempRef.Type == TokType.Int)
                && (t1.TempRef.Type == TokType.Double || t1.TempRef.Type == TokType.Int))
            {
                Token result = tacCalcConstant(t2, t1, op);
                BasicPrimitive tbp = new BasicPrimitive();
                var tmp = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, result.DType);
                tmp.TempRef = result;

                tbp.Expr = tmp.StringRep + " = " + result.StringRep;
                tbp.Polish.Add(tmp);
                tbp.Polish.Add(new Token("=", TokType.OperatorAssign));
                tbp.Polish.Add(result);
                tbp.mainOpIdx = 1;

                ++currentFun.tempVarCounter;

                tac.Remove(tac.Last());
                tac.Remove(tac.Last());
                tac.Add(tbp);

                ts.Push(tmp);
                return;
            }

            if (!isNumeric(getVarType(t1)) || !isNumeric(getVarType(t2)))
                throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString() + " or " + t2.DType.ToString(), null);
            uint type1 = (uint)t1.DType, type2 = (uint)t2.DType;

            //Token t1 = rpn[i - 1], t2 = rpn[i - 2];
            if (type1 < type2)
            {
                BasicPrimitive conv = new BasicPrimitive();
                t1 = convert(ref conv, t1, getVarType(t2));
                //rpn[i - 1] = t1;
                tac.Insert(tac.Count - 1, conv); //TEST
            }
            else if (type1 > type2)
                {
                    BasicPrimitive conv = new BasicPrimitive();
                    t2 = convert(ref conv, t2, getVarType(t1));
                    //rpn[i - 2] = t2;
                    tac.Add(conv); //TEST
                }

            var res = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, t1.DType);

            taccode.Polish.Add(res);
            taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
            taccode.Expr = "t" + currentFun.tempVarCounter + " = ";
            taccode.Polish.Add(t2);
            taccode.Polish.Add(new Token(op, TokType.Operator));
            taccode.Polish.Add(t1);
            taccode.mainOpIdx = 3;
            taccode.Expr += t2.StringRep;
            taccode.Expr += " " + op + " ";
            taccode.Expr += t1.StringRep;

            ts.Push(res);

            tac.Add(taccode);

            ++currentFun.tempVarCounter;
        }

        void tacEvalCast(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {
            var t1 = ts.Pop();           
            if (getVarType(tok.StringRep) == t1.DType)
            {
                ts.Push(t1);
                return;
            }
            BasicPrimitive conv = new BasicPrimitive();
            t1 = convert(ref conv, t1, getVarType(tok.StringRep), true);
            ts.Push(t1);
            tac.Add(conv);
        }

        void tacEvalNew(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {
            var res = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, DataTypes.Array);
            res.NewArrayDimens = tok.NewArrayDimens;
            res.BaseArrayType = tok.DType;

            var op = tok.StringRep;
            var taccode = new BasicPrimitive();

            taccode.Polish.Add(res);
            taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
            taccode.Expr = "t" + currentFun.tempVarCounter + " = ";
            ++currentFun.tempVarCounter;
            var t1 = ts.Pop();
            if (!isNumeric(t1.DType) && t1.DType != DataTypes.Double)
                throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString(), null);

            var tarr = new Token("newarr", TokType.OperatorNew, tok.DType);
            tarr.NewArrayDimens = tok.NewArrayDimens;
            taccode.Polish.Add(tarr);
            taccode.Polish.Add(t1);
            taccode.mainOpIdx = 2;
            taccode.Expr += "newarr " + tok.DType.ToString().ToLower() + " " + t1.StringRep;
            ts.Push(res);
            tac.Add(taccode);
            //rpn[i - 2] = res;
            //rpn.RemoveRange(i - 1, 2);
        }

        void tacEvalFun(Token tok, Stack<Token> ts, List<BasicPrimitive> tac, List<Token> rpn)
        {
            var op = tok.StringRep;
            var taccode = new BasicPrimitive();

            var scount = op.Remove(0, op.LastIndexOf("@") + 1);
            int count = int.Parse(scount);

            //count += 5;
            List<DataTypes> argTypes = new List<DataTypes>();

            //|t2|t1|t0|
            int ts_count = ts.Count - count;
            for (int t = 0; t < count; ++t)
            {
                argTypes.Add(getVarType(ts.ElementAt(t)));
            }
            argTypes.Reverse();
            Function f = getFunctionByArgList(op.Remove(op.LastIndexOf("@")), argTypes);
            var funName = f.name;
            if (f.module != "")
            {
                funName = "[" + f.module + "] " + f.name;
            }

            Token retval = null;

            if (f.type != DataTypes.Void && rpn.Count != 1)
            {
                retval = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, f.type);
                if(f.type == DataTypes.Array)
                {
                    retval.BaseArrayType = f.nested.type;
                    retval.NewArrayDimens = f.arrayDimens;
                }
                ++currentFun.tempVarCounter;

                taccode.Expr = retval.StringRep + " = _@call@_ " + funName + " (";
                //taccode.Type = retval.Type;
                taccode.Polish.Add(retval);
                taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                taccode.mainOpIdx = 2;
                taccode.Polish.Add(new Token("_@call@_", TokType.Keyword));
            }
            else if (f.type != DataTypes.Void && rpn.Count == 1 && noreturn)
                {
                    taccode.Expr = "_@call_noreturn@_ " + funName + " (";
                    taccode.mainOpIdx = 0;
                    taccode.Polish.Add(new Token("_@call_noreturn@_", TokType.Keyword));
                }
                else
                {
                    taccode.Expr = "_@call@_ " + funName + " (";
                    taccode.mainOpIdx = 0;
                    taccode.Polish.Add(new Token("_@call@_", TokType.Keyword));
                }

            taccode.Polish.Add(new Token(funName + "(" + f.ArgsFromList() + ")", TokType.Function));                   

            foreach (var a in f.argTypes)
            {
                taccode.Expr += a.type.ToString().ToLower() + ", ";
            }
            if (f.argTypes.Count != 0)
                taccode.Expr = taccode.Expr.Remove(taccode.Expr.Length - ", ".Length);
            taccode.Expr += ") ";
            for (int ac = 0; ac < count; ++ac)
            {
                var arg = ts.ElementAt(ac);
                if (getVarType(arg) == f.argTypes[f.argTypes.Count - ac - 1].type)
                {
                    taccode.Polish.Add(arg);
                    taccode.Expr += arg.StringRep + ", ";
                }
                else
                {
                    long none_var;
                    if (Int64.TryParse(arg.TempRef.StringRep, out none_var) && f.argTypes[f.argTypes.Count - ac - 1].type == DataTypes.Double)
                    {
                        ts.ElementAt(ac).DType = DataTypes.Double;
                        ts.ElementAt(ac).TempRef.Type = TokType.Double;
                        ts.ElementAt(ac).TempRef.DType = DataTypes.Double;
                        ts.ElementAt(ac).TempRef.StringRep += ".0";
                        taccode.Polish.Add(arg);
                        taccode.Expr += arg.StringRep + ", ";
                    }
                    else
                    {
                        var conv = new BasicPrimitive();
                        var tmp = convert(ref conv, arg, f.argTypes[f.argTypes.Count - ac - 1].type);
                        tac.Add(conv);
                        taccode.Polish.Add(tmp);
                        taccode.Expr += tmp.StringRep + ", ";
                    }
                    //rpn[t] = tmp;
                }
            }

            for (; count > 0; --count)
                ts.Pop();

            if (f.argTypes.Count != 0)
                taccode.Expr = taccode.Expr.Remove(taccode.Expr.Length - ", ".Length);

            //rpn.RemoveRange(i - count, count + 1);
            if (retval != null)
                ts.Push(retval);
            tac.Add(taccode);
        }

        void tacEvalArrElem(Token tok, Stack<Token> ts, List<BasicPrimitive> tac)
        {
            var res = new Token("t" + currentFun.tempVarCounter, TokType.TempVar);

            var taccode = new BasicPrimitive();

            var t2 = ts.Pop();
            var t1 = ts.Pop();

            if (t1.DType != DataTypes.Array)
                throw new CompilerException(ExceptionType.IllegalType, "Array ecpected", null);
            if (!new DataTypes[] {DataTypes.Byte, DataTypes.Short, DataTypes.Int, DataTypes.Long, DataTypes.Uint, DataTypes.Ulong}
                    .Contains(t2.DType))
                throw new CompilerException(ExceptionType.IllegalType, "Integer value ecpected as array index", null);
            if (t1.NewArrayDimens == 1)
                res.DType = t1.BaseArrayType;
            else
            {
                res.DType = DataTypes.Array;
                res.BaseArrayType = t1.BaseArrayType;
                res.NewArrayDimens = t1.NewArrayDimens-1;
            }
            res.TempRef = new Token("[]", TokType.ArrayElem);

            taccode.Polish.Add(res);
            taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
            taccode.Expr = "t" + currentFun.tempVarCounter + " = ";
            ++currentFun.tempVarCounter;

            taccode.Polish.Add(t2);
            taccode.Polish.Add(new Token("[]", TokType.ArrayElemOp));
            taccode.Polish.Add(t1);
            taccode.mainOpIdx = 3;
            taccode.Expr += t2.StringRep + " " + "[]" + " " + t1.StringRep;
            ts.Push(res);
            tac.Add(taccode);
            //rpn[i - 2] = res;
            //rpn.RemoveRange(i - 1, 2);
        }

        Token tacCalcConstant(Token t1, Token t2, string op)
        {
            bool _double = false;
            string s2 = t2.TempRef.StringRep, s1 = t1.TempRef.StringRep;
            if (t1.DType == DataTypes.Double)
            {
                _double = true;
                s1 = t1.TempRef.StringRep;
                if (t2.DType != DataTypes.Double)
                {
                    s2 = t2.TempRef.StringRep + ".0";
                }
            }
            if (t2.DType == DataTypes.Double)
            {
                s2 = t2.TempRef.StringRep;
                if (t1.DType != DataTypes.Double)
                {
                    _double = true;
                    s1 = t1.TempRef.StringRep + ".0";
                }
            }
            string res = "";    

            switch (op)
            {
                case "+":
                    {
                        if (_double)
                        {
                            res = "" + (double.Parse(s2, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture)
                                + double.Parse(s1, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture));
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) + int.Parse(s1));
                        }
                    }
                    break;
                case "-":
                    {
                        if (_double)
                        {
                            res = "" + (double.Parse(s2, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture)
                                - double.Parse(s1, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture));
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) - int.Parse(s1));
                        }
                    }
                    break;
                case "*":
                    {
                        if (_double)
                        {
                            res = "" + (double.Parse(s2, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture)
                                * double.Parse(s1, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture));
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) * int.Parse(s1));
                        }
                    }
                    break;
                case "/":
                    {
                        if (_double)
                        {
                            res = "" + (double.Parse(s2, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture)
                                / double.Parse(s1, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture));
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) / int.Parse(s1));
                        }
                    }
                    break;
                case "%":
                    {
                        if (_double)
                        {
                            res = "" + (double.Parse(s2, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture)
                                % double.Parse(s1, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture));
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) % int.Parse(s1));
                        }
                    }
                    break;
                case "|":
                    {
                        if (_double)
                        {
                            throw new CompilerException(ExceptionType.IllegalType, "operator `|` can't be applied to type `double", null);
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) | int.Parse(s1));
                        }
                    }
                    break;
                case "&":
                    {
                        if (_double)
                        {
                            throw new CompilerException(ExceptionType.IllegalType, "operator `&` can't be applied to type `double", null);
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) & int.Parse(s1));
                        }
                    }
                    break;
                case "^":
                    {
                        if (_double)
                        {
                            throw new CompilerException(ExceptionType.IllegalType, "operator `^` can't be applied to type `double", null);
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) | int.Parse(s1));
                        }
                    }
                    break;
                case "<<":
                    {
                        if (_double)
                        {
                            throw new CompilerException(ExceptionType.IllegalType, "operator `<<` can't be applied to type `double", null);
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) << int.Parse(s1));
                        }
                    }
                    break;
                case ">>":
                    {
                        if (_double)
                        {
                            throw new CompilerException(ExceptionType.IllegalType, "operator `>>` can't be applied to type `double", null);
                        }
                        else
                        {
                            res = "" + (int.Parse(s2) >> int.Parse(s1));
                        }
                    }
                    break;
                default:
                    break;
            }
            res = res.Replace(",", ".");
            Token tres = new Token(res, _double ? TokType.Double : TokType.Int, _double ? DataTypes.Double : DataTypes.Int);
            return tres;
        }
        /*
        string tacCalc<T>(T t1, T t2, string op)
        {
            string sres = "";
            T res;
            switch (op)
            {
                case "+":
                {
                    res = t1 + t2;
                }
                    break;
                case "-":
                {
                    res = t1 - t2;
                }
                    break;
                case "*":
                {
                    res = t1 * t2;
                }
                    break;
                case "/":
                {
                    res = t1 / t2;
                }
                    break;
                case "%":
                {
                    res = t1 % t2;   
                }
                    break;
                default:
                    break;
            }

        }
        */
        void tacSetType(BasicPrimitive bp)
        {
            byte type = 0;
            var tempt = new TokType[] { TokType.Boolean, TokType.Int, TokType.Double, TokType.TempVar, TokType.Variable };
            foreach (var tok in bp.Polish)
            {

                if (tempt.Contains(tok.Type))
                {
                    var t = (byte)getVarType(tok);
                    if (t > type)
                        type = t;
                }
            }
            bp.Type = (DataTypes)type;
        }

        List<BasicPrimitive> evalTAC(BasicPrimitive bp)
        {
            List<BasicPrimitive> tac = new List<BasicPrimitive>();

            var rpn = bp.Polish;
            //if(rpn.Count == 1 && rpn[0].Type == TokType.Function)
            while (rpn.Count > 1 || (rpn.Count == 1 && rpn[0].Type == TokType.Function))
            {
                int i = 0;
                string op = "";
                TokType type = new TokType();
                for (; i < rpn.Count; ++i)
                {
                    if (bp.Polish[i].Type == TokType.Operator || rpn[i].Type == TokType.Function)
                    {
                        op = rpn[i].StringRep;
                        type = rpn[i].Type;
                        break;
                    }
                }
                if (type == TokType.Function)
                {
                    //Token tok = new Token ();

                    var scount = op.Remove(0, op.LastIndexOf("@") + 1);
                    int count = int.Parse(scount);

                    //count += 5;
                    List<DataTypes> argTypes = new List<DataTypes>();

                    for (int t = i - count; t < i; ++t)
                    {
                        argTypes.Add(getVarType(rpn[t]));
                    }

                    Function f = getFunctionByArgList(op.Remove(op.LastIndexOf("@")), argTypes);
                    var funName = f.name;
                    if (f.module != "")
                    {
                        funName = "[" + f.module + "] " + f.name;
                    }
                    var taccode = new BasicPrimitive();
                    if (f.type != DataTypes.Void && rpn.Count != 1)
                    {
                        Token retval = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, f.type);
                        ++currentFun.tempVarCounter;

                        taccode.Expr = retval.StringRep + " = _@call@_ " + funName + " (";
                        //taccode.Type = retval.Type;
                        taccode.Polish.Add(retval);
                        taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                        taccode.mainOpIdx = 2;
                        taccode.Polish.Add(new Token("_@call@_", TokType.Keyword));
                    }
                    else if (f.type != DataTypes.Void && rpn.Count == 1)
                        {
                            taccode.Expr = "_@call_noreturn@_" + funName + " (";
                            taccode.mainOpIdx = 0;
                            taccode.Polish.Add(new Token("_@call_noreturn@_", TokType.Keyword));
                        }
                        else
                        {
                            taccode.Expr = "_@call@_ " + funName + " (";
                            taccode.mainOpIdx = 0;
                            taccode.Polish.Add(new Token("_@call@_", TokType.Keyword));
                        }
                    //taccode.Polish.Add(new Token { StringRep = "_@call@_", Type = TokType.Keyword });
                    taccode.Polish.Add(new Token(funName + "(" + f.ArgsFromList() + ")", TokType.Function));                   

                    foreach (var a in f.argTypes)
                    {
                        taccode.Expr += a.type.ToString().ToLower() + ", ";
                    }
                    if (f.argTypes.Count != 0)
                        taccode.Expr = taccode.Expr.Remove(taccode.Expr.Length - ", ".Length);
                    taccode.Expr += ") ";
                    for (int t = i - count, ac = 0; t < i; ++t, ++ac)
                    {
                        if (getVarType(rpn[t]) == f.argTypes[ac].type)
                        {
                            taccode.Polish.Add(rpn[t]);
                            taccode.Expr += rpn[t].StringRep + ", ";
                        }
                        else
                        {
                            var conv = new BasicPrimitive();
                            var tmp = convert(ref conv, rpn[t], f.argTypes[ac].type);
                            tac.Add(conv);
                            taccode.Polish.Add(tmp);
                            taccode.Expr += tmp.StringRep + ", ";
                            rpn[t] = tmp;
                        }
                    }
                    if (f.argTypes.Count != 0)
                        taccode.Expr = taccode.Expr.Remove(taccode.Expr.Length - ", ".Length);

                    rpn.RemoveRange(i - count, count + 1);
                    if (f.type != DataTypes.Void)
                        rpn.Insert(i - count, new Token("t" + (currentFun.tempVarCounter - 1), TokType.TempVar, f.type));
                    tac.Add(taccode);


                    /*
                        int var = somefun(10, "hello", 3.14, true);
                        t1 = 10 as double
                        t2 = 3.14 as i32
                        t3 =  _@call@_ somefun(double, string, int, bool) t1, "hello", t2, true 
                        var = t3 as int
                     */

                }
                else if (type == TokType.Operator)
                    {
                        var taccode = new BasicPrimitive();
                        var testOp = new string [] { "==", "!=", "<", "<=", ">=", ">" };

                        if (op.StartsWith("#") || op == "++" || op == "--")
                        {
                            Token tmp;
                            if (op == "#!")
                            {
                                tmp = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, DataTypes.Bool);
                                taccode.Polish.Add(tmp);
                                taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                                taccode.Polish.Add(new Token(op, TokType.OperatorMono));
                                taccode.mainOpIdx = 2;
                                if (getVarType(rpn[i - 1]) == DataTypes.Bool)
                                {
                                    taccode.Expr = tmp.StringRep + " = " + op + " " + rpn[i - 1].StringRep;
                                    //taccode.Type = DataTypes.Bool;
                                    taccode.Polish.Add(rpn[i - 1]);
                                    ++currentFun.tempVarCounter;
                                }
                                else
                                {
                                    var conv = new BasicPrimitive();
                                    var temp = convert(ref conv, rpn[i - 1], DataTypes.Bool);
                                    tac.Add(conv);
                                    taccode.Polish.Add(temp);
                                    taccode.Expr = tmp.StringRep + " = " + op + " " + temp.StringRep;
                                    rpn[i - 1] = temp;
                                }
                            }
                            else
                            {
                                if (!isNumeric(getVarType(rpn[i - 1])))
                                    throw new CompilerException(ExceptionType.NonNumericValue, "Numeric value expected!", null);

                                tmp = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, getVarType(rpn[i - 1]));
                                taccode.Expr = tmp.StringRep + " = " + op + " " + rpn[i - 1].StringRep;
                                //taccode.Type = getVarType(rpn[i - 1]);
                                taccode.Polish.Add(tmp);
                                taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                                taccode.Polish.Add(new Token(op, TokType.OperatorMono));
                                taccode.Polish.Add(rpn[i - 1]);
                                taccode.mainOpIdx = 2;
                                ++currentFun.tempVarCounter;
                            }
                            rpn.RemoveAt(i);
                            rpn.Insert(i, tmp);
                            rpn.RemoveAt(i - 1);
                        }
                        else if (testOp.Contains(op))
                            {
                                var res = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, DataTypes.Bool);
                                taccode.Polish.Add(res);
                                taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                                taccode.Expr = "t" + currentFun.tempVarCounter + " = ";
                                ++currentFun.tempVarCounter;
                                var t1 = rpn[i - 1];
                                var t2 = rpn[i - 2];
                                if (t1.DType == DataTypes.Void || t2.DType == DataTypes.Void)
                                    throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString() + " or " + t2.DType.ToString(), null);

                                var conv = new BasicPrimitive();
                                if (op == "==" || op == "!=")
                                {
                                    if (t1.DType != t2.DType)
                                    {
                                        //var conv = new BasicPrimitive();
                                        if (t1.DType > t2.DType) //TEST
                                        {
                                            t2 = convert(ref conv, t2, t1.DType);
                                            rpn[i - 2] = t2;
                                        }
                                        else
                                        {
                                            t1 = convert(ref conv, t1, t2.DType);
                                            rpn[i - 1] = t1;
                                        }
                                        tac.Add(conv);
                                    }

                                }
                                else if (!isNumeric(t1.DType) || !isNumeric(t2.DType))
                                        throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString() + " or " + t2.DType.ToString(), null);
                                    else if (t1.DType > t2.DType)
                                        {
                                            t2 = convert(ref conv, t2, t1.DType);
                                            rpn[i - 2] = t2;
                                            tac.Add(conv);
                                        }
                                        else if (t1.DType < t2.DType)
                                            {
                                                t1 = convert(ref conv, t1, t2.DType);
                                                rpn[i - 1] = t1;
                                                tac.Add(conv);
                                            }
                                taccode.Polish.Add(t2);
                                taccode.Polish.Add(new Token(op, TokType.Operator));
                                taccode.Polish.Add(t1);
                                taccode.mainOpIdx = 3;
                                taccode.Expr += t2.StringRep + " " + op + " " + t1.StringRep;
                                rpn[i - 2] = res;
                                rpn.RemoveRange(i - 1, 2);
                            }
                            else if (op == "&&" || op == "||")
                                {
                                    var res = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, DataTypes.Bool);
                                    taccode.Polish.Add(res);
                                    taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                                    taccode.Expr = "t" + currentFun.tempVarCounter + " = ";
                                    ++currentFun.tempVarCounter;
                                    if (isNumeric(getVarType(rpn[i - 2])))
                                    {
                                        var conv = new BasicPrimitive();
                                        var temp = convert(ref conv, rpn[i - 2], DataTypes.Bool);
                                        tac.Add(conv);
                                        taccode.Polish.Add(temp);
                                        taccode.Expr += temp.StringRep;
                                        rpn[i - 2] = temp;
                                        //taccode.Expr += temp.StringRep;
                                    }
                                    else if (getVarType(rpn[i - 2]) == DataTypes.Bool)
                                        {
                                            taccode.Polish.Add(rpn[i - 2]);
                                            taccode.Expr += rpn[i - 2].StringRep;
                                        }
                                        else
                                            throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + rpn[i - 2].DType.ToString(), null);
                                    taccode.Polish.Add(new Token(op, TokType.Operator));
                                    taccode.Expr += " " + op + " ";
                                    if (isNumeric(getVarType(rpn[i - 1])))
                                    {
                                        var conv = new BasicPrimitive();
                                        var tmp = convert(ref conv, rpn[i - 1], DataTypes.Bool);
                                        tac.Add(conv);
                                        taccode.Polish.Add(tmp);
                                        taccode.Expr += tmp.StringRep;
                                        rpn[i - 1] = tmp;
                                        //taccode.Expr += tmp.StringRep;
                                    }
                                    else if (getVarType(rpn[i - 1]) == DataTypes.Bool)
                                        {
                                            taccode.Polish.Add(rpn[i - 1]);
                                            taccode.Expr += rpn[i - 1].StringRep;
                                        }
                                        else
                                            throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + rpn[i - 1].DType.ToString(), null);
                                    rpn[i - 2] = res;
                                    rpn.RemoveRange(i - 1, 2);
                                    taccode.mainOpIdx = 3;
                                }
                                else if (op == "=")
                                    {
                                        Token t1 = rpn[i - 1], t2 = rpn[i - 2];
                                        uint dt1 = (uint)t1.DType, dt2 = (uint)t2.DType;

                                        //dt1 = 3 - dt2 = 3;

                                        if (dt1 != dt2 && canCastType(t1.DType, t2.DType))
                                        {
                                            var conv = new BasicPrimitive();
                                            t1 = convert(ref conv, t1, t2.DType);
                                            rpn[i - 1] = t1;
                                            tac.Add(conv);
                                        }
                                        else if (dt1 != dt2)
                                                throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + t1.DType.ToString() + " or " + t2.DType.ToString(), null);

                                        taccode.Expr = t2.StringRep + " = " + t1.StringRep;
                                        taccode.Polish.Add(t2);
                                        taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                                        taccode.Polish.Add(t1);
                                        taccode.mainOpIdx = 1;
                                        rpn[i] = t2;
                                        rpn.RemoveAt(i - 1);
                                        rpn.RemoveAt(i - 2);
                                    }
                                    else
                                    {
                                        if (!isNumeric(getVarType(rpn[i - 1])) || !isNumeric(getVarType(rpn[i - 2])))
                                            throw new CompilerException(ExceptionType.IllegalType, "Illegal type: " + rpn[i - 1].DType.ToString() + " or " + rpn[i - 2].DType.ToString(), null);
                                        uint type1 = (uint)rpn[i - 1].DType, type2 = (uint)rpn[i - 2].DType;

                                        Token t1 = rpn[i - 1], t2 = rpn[i - 2];
                                        if (type1 < type2)
                                        {
                                            BasicPrimitive conv = new BasicPrimitive();
                                            t1 = convert(ref conv, t1, getVarType(t2));
                                            rpn[i - 1] = t1;
                                            tac.Add(conv);
                                        }
                                        else if (type1 > type2)
                                            {
                                                BasicPrimitive conv = new BasicPrimitive();
                                                t2 = convert(ref conv, t2, getVarType(t1));
                                                rpn[i - 2] = t2;
                                                tac.Add(conv);
                                            }

                                        taccode.Polish.Add(new Token("t" + currentFun.tempVarCounter, TokType.TempVar, t1.DType));
                                        taccode.Polish.Add(new Token("=", TokType.OperatorAssign));
                                        taccode.Expr = "t" + currentFun.tempVarCounter + " = ";
                                        taccode.Polish.Add(t2);
                                        taccode.Polish.Add(new Token(op, TokType.Operator));
                                        taccode.Polish.Add(t1);
                                        taccode.mainOpIdx = 3;
                                        taccode.Expr += t2.StringRep;
                                        taccode.Expr += " " + op + " ";
                                        taccode.Expr += t1.StringRep;

                                        rpn[i] = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, t1.DType);
                                        rpn.RemoveAt(i - 1);
                                        rpn.RemoveAt(i - 2);
                                        ++currentFun.tempVarCounter;
                                    }
                        tac.Add(taccode);
                        if (op == "++" || op == "--")
                        {
                            var assignment = new BasicPrimitive();
                            assignment.Expr = taccode.Polish.Last().StringRep + " = " + taccode.Polish[0].StringRep;
                            assignment.Polish.Add(taccode.Polish.Last());
                            assignment.Polish.Add(new Token("=", TokType.OperatorAssign));
                            assignment.Polish.Add(taccode.Polish[0]);
                            assignment.mainOpIdx = 1;
                            tac.Add(assignment);
                        }
                    }

            }
            if (rpn.Count != 0)
            {
                var last = new BasicPrimitive();
                last.Polish.Add(rpn[0]);
                last.Expr = rpn[0].StringRep;
                tac.Add(last);
            }
            return tac;
        }

        Token convert(ref BasicPrimitive conv, Token from, DataTypes to, bool check = false)
        {
            //Console.WriteLine("DEBUG: converting type '" + from.DType.ToString() + "' to '" + to.ToString()+"'");
            //TEST:
            if (from.DType == to)
            {
                //Console.WriteLine("DEBUG: type '" + from.DType.ToString() + "' == '" + to.ToString()+"'");
                return from;
            }

            if (check)
            {
                if (!canCastType(from.DType, to) && !canCastType(to, from.DType))
                    throw new CompilerException(ExceptionType.IllegalCast, "Illegal cast from " + from.DType.ToString() + " to " + to.ToString(), null);
            }
            var tmp = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, to);
            conv.Expr = "t" + currentFun.tempVarCounter + " = " + from.StringRep + " _@as@_ " + to.ToString().ToLower();
            conv.Polish.Add(tmp);
            conv.Polish.Add(new Token("=", TokType.OperatorAssign));
            conv.Polish.Add(from);
            conv.Polish.Add(new Token("_@as@_", TokType.Operator));
            conv.Polish.Add(new Token(to.ToString().ToLower(), TokType.Keyword, to));
            conv.mainOpIdx = 3;
            ++currentFun.tempVarCounter;
            return tmp;
        }

        bool isNumeric(DataTypes dt)
        {
            var testList = new List<DataTypes>
            {
                DataTypes.Byte, DataTypes.Short, DataTypes.Int, DataTypes.Uint, 
                DataTypes.Long, DataTypes.Ulong, DataTypes.Double
            };
            return testList.Contains(dt);
        }

        DataTypes getVarType(Token tok)
        {
            switch (tok.Type)
            {
                case TokType.Variable:
                    {
                        int id = VarType.GetVarID(tok.StringRep);
                        if (VarType.GetVarPrefix(tok.StringRep) == VarType.LOCAL_PREFIX)
                        {
                            return currentFun.locals[id].type;
                        }
                        else if (VarType.GetVarPrefix(tok.StringRep) == VarType.ARG_PREFIX)
                            {
                                return currentFun.argTypes[id].type;
                            }
                            else if (VarType.GetVarPrefix(tok.StringRep) == VarType.FIELD_PREFIX)
                                {
                                    return globalVars.Locals[id].Var.type;
                                }
                        break;
                    }
                case TokType.TempVar:
                    return tok.DType;
                case TokType.Boolean:
                    return DataTypes.Bool;
                case TokType.Double:
                    return DataTypes.Double;
                case TokType.Int:
                    return DataTypes.Int;
                case TokType.String:
                    return DataTypes.String;
                default:
                    throw new Exception("Unknown type '" + tok.Type.ToString() + "'");
            }
            return DataTypes.Null;
        }

        DataTypes getVarType(string str)
        {
            var upper = str[0].ToString().ToUpper();
            var t = upper + str.Remove(0, 1);
            return (DataTypes)Enum.Parse(typeof(DataTypes), t);
        }

        Function getFunctionByArgList(string name, List<DataTypes> argTypes)
        {
            List<Function> overloaded = new List<Function>();
            foreach (var f in funcs)
            {
                if (f.name == name && f.argTypes.Count == argTypes.Count)
                    overloaded.Add(f);
            }
            foreach (var m in imported)
            {
                foreach (var f in m.functions)
                    if (f.name == name && f.argTypes.Count == argTypes.Count)
                        overloaded.Add(f);
            }
            if (overloaded.Count == 1)
            {
                for (int i = 0; i < argTypes.Count; ++i)
                {
                    if (argTypes[i] != overloaded[0].argTypes[i].type)
                    {
                        if (!canCastType(argTypes[i], overloaded[0].argTypes[i].type))
                            throw new CompilerException(ExceptionType.IllegalType, "Can't find function: " + name, null);
                    }
                }
                return overloaded[0];
            }
            bool isEq = true;
            foreach (var f in overloaded.ToList())
            {
                var canCast = true;
                for (int i = 0; i < argTypes.Count; ++i)
                {
                    if (argTypes[i] != f.argTypes[i].type)
                    {
                        isEq = false;
                        if (!canCastType(argTypes[i], f.argTypes[i].type))
                        {
                            canCast = false;
                            overloaded.Remove(f);
                            break;
                        }
                    }
                    else
                        isEq = true;
                }
                if (isEq)
                    return f;
            }
            if (overloaded.Count == 1)
                return overloaded[0];
            else
                throw new CompilerException(ExceptionType.FunctionRedefinition, "Unexpected ambigious overload: " + name, null);
        }

        bool canCastType(DataTypes from, DataTypes to)
        {
            List<DataTypes> numeric = new List<DataTypes>
            {DataTypes.Byte, DataTypes.Short, DataTypes.Int, DataTypes.Uint, 
                DataTypes.Long, DataTypes.Ulong, DataTypes.Double
            };
            if (from == DataTypes.Double && to != DataTypes.Double)
                return false;
            else if (isNumeric(from) && isNumeric(to))
                    return true;
                else if (isNumeric(from) && to == DataTypes.Bool)
                        return true;
            return false;
        }

        private bool checkTopOp(string p, Token top, bool isUnary)
        {
            // a = b = c = 5
            // a b c 5 = = =
            if (top.Type != TokType.Operator)
                return false;

            if (p == "=")
                return priorities.GetPriority(p) < priorities.GetPriority(top.StringRep);
            if (isUnary && p != "cast")
                if (p == "+" || p == "-" || p == "!" || p == "~")
                    return priorities.GetPriority("#" + p) < priorities.GetPriority(top.StringRep);
                else
                    throw new CompilerException(ExceptionType.UnknownOp, "Unknown operator " + p, null);
            else if (priorities.GetPriority(p) <= priorities.GetPriority(top.StringRep))
                    return true;

            return false;
        }

        private CodeBlock evalSingleStatement(string expr, CodeBlock parent)
        {
            return evalSingleStatement(new TokenStream(expr), parent);
        }

        private CodeBlock evalSingleStatement(TokenStream toks, CodeBlock parent, bool current = false, bool inswitch = false)
        {
            CodeBlock blk = current ? parent : new CodeBlock(parent);
            if (toks.Next() == ";" && toks.Type == TokenType.Separator)
            {
                blk.Inner.Add(null);
                return blk;
            }
            /*switch (toks.ToString())
            {
                case "break":
                    evalBreak(toks, blk);
                    break;
                case "continue":
                    evalContinue(toks, blk);
                    break;
                case "for":
                    evalFor(toks, blk);
                    break;
                case "if":
                    evalIf(toks, blk);
                    break;
                case "while":
                    evalWhile(toks, blk);
                    break;
                case "return":
                    evalReturn(toks, blk);
                    break;
                case "goto":
                    evalGoto(toks, blk);
                    break;
                case "{":
                    toks.PushBack();
                    evalBlock(toks, blk, true);
                    toks.Next();
                    return blk;
                case "}":
                    return blk;
                default:
                    if (isType(toks.ToString()))
                    {
                        //toks.PushBack();
                        evalLocalVar(toks.ToString(), toks, blk);
                    }
                    else
                        evalExpr(toks, blk);
                    break;
            }*/
            if (CommandArgs.lang == SyntaxLang.English)
                switch (toks.ToString())
                {
                    case "break":
                        evalBreak(toks, blk);
                        break;
                    case "continue":
                        evalContinue(toks, blk);
                        break;
                    case "for":
                        evalFor(toks, blk);
                        break;
                    case "if":
                        evalIf(toks, blk);
                        break;
                    case "switch":
                        evalSwitch(toks, blk);
                        break;
                    case "while":
                        evalWhile(toks, blk);
                        break;
                    case "return":
                        evalReturn(toks, blk);
                        break;
                    case "do":                
                        evalDoWhile(toks, blk);
                        break;
                    case "goto":
                        evalGoto(toks, blk);
                        break;
                    case "{":
                        toks.PushBack();
                        noreturn = true;
                        evalBlock(toks, blk, true); 
                        noreturn = false;
                        toks.Next();
                        return blk;
                    case "}":
                        return blk;
                    default:
                        if (isType(toks.ToString()))
                        {
                            //toks.PushBack();
                            evalLocalVar(toks.ToString(), toks, blk);
                        }
                        else
                        {
                            if ((toks.ToString() == "case" || toks.ToString() == "default") && inswitch)
                                return blk;
                            evalExpr(toks, blk);
                            noreturn = false;
                        }
                        break;
                }
            else
                switch (toks.ToString())
                {
                    case "зупинити":
                        evalBreak(toks, blk);
                        break;
                    case "продовжити":
                        evalContinue(toks, blk);
                        break;
                    case "для":
                        evalFor(toks, blk);
                        break;
                    case "якщо":
                        evalIf(toks, blk);
                        break;
                    case "поки":
                        evalWhile(toks, blk);
                        break;
                    case "повернути":
                        evalReturn(toks, blk);
                        break;
                    case "перейти":
                        evalGoto(toks, blk);
                        break;
                    case "робти":
                        evalDoWhile(toks, blk);
                        break;
                    case "{":
                        toks.PushBack();
                        evalBlock(toks, blk, true);
                        toks.Next();
                        return blk;
                    case "}":
                        return blk;
                    default:
                        if (isType(toks.ToString()))
                        {
                            //toks.PushBack();
                            evalLocalVar(toks.ToString(), toks, blk);
                        }
                        else
                        {
                            noreturn = true;
                            evalExpr(toks, blk);
                            noreturn = false;
                        }
                        break;
                }
            toks.Next();
            return blk;
        }

        private bool isType(string p)
        {
            return isPODType(p);
        }

        private void evalLocalVar(string type, TokenStream toks, CodeBlock blk)
        {
            evalLocalVar(type, toks, blk.Inner, blk.Locals, blk);
        }

        private void evalLocalVar(string type, TokenStream toks, List<object> inner, List<VarDecl> vars, CodeBlock blk)
        {
            while (true)
            {

                // int [] arr === Array<int> arr
                // int [][][]arr == Array<Array<Arrray<int>>> arr
                //string type = toks.Next();
                bool isArray = false;
                int dimens = 1;

                if (toks.Next() == "[")
                {
                    isArray = true;
                    if (toks.Next() != "]")
                        throw new CompilerException(ExceptionType.Brace, "`]` expected in array declaration", toks);
                    while (true)
                    {
                        if (toks.Next() == "[")
                            dimens++;
                        else
                        {
                            toks.PushBack();
                            break;
                        }
                        if (toks.Next() != "]")
                            throw new CompilerException(ExceptionType.Brace, "`]` expected in array declaration", toks);
                    }
                }
                else
                    toks.PushBack();

                string ident = getIdentifierNext(toks);
                var name = ident;
                CodeBlock b = blk.Parent;
                int level = 0;
                while (b != null)
                {
                    ++level;
                    b = b.Parent;
                }
                ident = VarType.LOCAL_PREFIX + ident;
                //VarType vt = new VarType(ident, getPODType(type), true); // Not supports STRUCTs
                VarType vt;
                if(isArray)
                    vt = new VarType(ident, DataTypes.Array, dimens, new DataType(getPODType(type))); // Not supports STRUCTs
                else
                    vt = new VarType(ident, getPODType(type)); // Not supports STRUCTs
                VarDecl vd = new VarDecl();
                vd.Var = vt;
                vd.LocalIndex = currentFun.locals.Count;
                inner.Add(vd);
                vars.Add(vd);
                currentFun.locals.Add(currentFun.locals.Count, vt);
                //var identId = VarType.LOCAL_PREFIX + vd.LocalIndex;
                if (toks.Next() == ";" && toks.Type == TokenType.Separator)
                {
                    return;
                }
                else if (toks.ToString() == "=")
                    {
                        string expr = name + " = ";
                        int brc = 0;
                        while (true)
                        {
                            if ((toks.Next() == "," && brc == 0) || (toks.ToString() == ";" && toks.Type == TokenType.Separator))
                                break;
                            expr += toks.ToString() + " ";
                            if (toks.Type != TokenType.String)
                            {
                                if (toks.ToString() == "(")
                                    ++brc;
                                else if (toks.ToString() == ")")
                                        --brc;
                            }
                        }
                    inner.Add(evalExpr(expr.Trim(), blk)); // Not supports STRUCTs
                    }
                if (toks.ToString() == ",")
                    continue;
                break;
            }
        }

        private void evalGlobalVar(string ident, DataType t, string sname)
        {
            string expr = ident + " = ";
            ident = VarType.FIELD_PREFIX + ident;
            VarType v;
            if (t.type != DataTypes.User)
                v = new VarType(ident, t);
            else
                throw new NotImplementedException();
                //v = new VarType(ident, t, new StructType(""));
        

            v.isNative = isNative;
            v.isPublic = isPublic;

            if (tokens.Type == TokenType.Separator)
            {
                VarDecl vd = new VarDecl();
                vd.LocalIndex = -1;
                vd.Var = v;
                globalVars.Locals.Add(vd);
                return;
            }
            else if (tokens.ToString() != "=")
                throw new CompilerException(ExceptionType.IllegalToken, "';' or '=' expected", tokens);

            do
            {
                expr += tokens.Next() + " ";
            } while (tokens.Type != TokenType.Separator && tokens.Type != TokenType.EOF);

            funcs[0].body += expr + "\n";
            VarDecl vdg = new VarDecl();
            vdg.Var = v;
            vdg.LocalIndex = -1;
            globalVars.Locals.Add(vdg);
            return;

            /* ~~~ DEPRECATED ~~~ */

            /*var val = tokens.Next();
            if (tokens.Type == TokenType.Digit && v.isDigitType())
            {
                if (val.Contains("."))
                {
                    v.val = Double.Parse(val.Replace(".", ","));
                }
                else if (val.StartsWith("-") && v.type == DataTypes.Uint)
                {
                    uint m = (uint)Math.Abs(Int32.Parse(val));
                    v.val = UInt32.MaxValue - m;
                }
                else if (val.StartsWith("-") && v.type == DataTypes.Ulong)
                {
                    ulong m = (ulong)Math.Abs(Int64.Parse(val));
                    v.val = UInt64.MaxValue - m;
                }
                else
                {
                    //long data = long.Parse(val);
                    switch (v.type)
                    {
                        case DataTypes.Byte:
                            v.val = Byte.Parse(val);
                            break;
                        case DataTypes.Short:
                            v.val = Int16.Parse(val);
                            break;
                        case DataTypes.Int:
                            v.val = Int32.Parse(val);
                            break;
                        case DataTypes.Double:
                            v.val = Double.Parse(val);
                            break;
                        case DataTypes.Ulong:
                            v.val = UInt64.Parse(val);
                            break;
                        case DataTypes.Uint:
                            v.val = UInt32.Parse(val);
                            break;
                        case DataTypes.Long:
                            v.val = Int64.Parse(val);
                            break;
                    }
                }
            }
            else if (tokens.Type == TokenType.String && v.type == DataTypes.String)
                    v.val = val;
            else if (tokens.Type == TokenType.Boolean && v.type == DataTypes.Bool)
                    v.val = Boolean.Parse(val);
            else
                throw new CompilerException(ExceptionType.IllegalType, "Invalid type difference in assignment statement", tokens);
            checkEosNext(tokens);*/
        }

        private CodeBlock evalBlock(TokenStream toks, CodeBlock parent, bool isCurrent = false)
        {
            return compileBlock(toks, parent, isCurrent);
        }

        private void evalGoto(string str, CodeBlock parent)
        {
            var ts = new TokenStream(str);
            ts.Next();
            evalGoto(ts, parent);
        }

        private void evalGoto(TokenStream toks, CodeBlock parent)
        {
            Goto g = new Goto();
            g.Label = getIdentifierNext(toks);
            parent.Inner.Add(g);
            checkEosNext(toks);
        }
        // (int x, int y) => int, {return x + y;};
        // lambda (int x, int y) of int => {return x + y;};
        // lambda (int x, int y) => int, {return x + y;};
        // [](int x, int y) => int, {return x + y;};
        private void evalWhile(TokenStream toks, CodeBlock parent)
        {            
            /*
                while (i < 0) {
                    printf("Hello!");
                    break;
                }

                _in:
                if(!(i<0)) goto _out;
                    printf("Hello!");
                    goto _else();
                {
                    //...
                    goto _out:
                }
                _else():
                {
                }
                _out:
             */
            if (toks.Next() != "(")
                throw new CompilerException(ExceptionType.Brace, "Brace '(' expected after 'while' statement", toks);
            var cond = "";
            int brc = 1;
            toks.Next();
            while (true)
            {
                if (toks.ToString() == ")")
                    --brc;
                else if (toks.ToString() == "(")
                        ++brc;
                if (brc == 0)
                    break;

                cond += toks.ToString();
                toks.Next();
            }
            var begin_while = "_while_label_" + while_label_counter;
            var end_while = "_end_while_label_" + while_label_counter;
            ++while_label_counter;
            currentFun.cycleStack.Push(new Tuple<string, string>(begin_while, end_while));

            //evalExpr("", parent);

            While w = new While(null, null);
            var body = new CodeBlock(parent);

            cond = localize("if") + "(" + cond + ") { " + localize("goto") + " " + end_while + " ; }"; 
            w.Body = new CodeBlock(parent);
            evalLabel(begin_while + ": ;", body);
            var condts = new TokenStream(cond);
            condts.Next();
            evalIf(condts, body);
            evalSingleStatement(toks, body, true);

            /*if (toks.Next() == "{")
            {
                toks.PushBack();
                evalBlock(toks, body, true);
                toks.Next ();
            }
            else
            {
                string str = "";
                while (!isEos(toks))
                {
                    str += toks.ToString();
                    toks.Next();
                }
                evalExpr(str, parent);
            }*/
            var gts = new TokenStream(localize("goto") + " " + begin_while + ";");
            gts.Next();
            evalGoto(gts, body);

            evalLabel(end_while + ": ;", body);

            if (toks.ToString() == localize("else"))
                evalSingleStatement(toks, body, true);

            evalLabel(end_while + "_2: ;", body);

            w.Body = body;
            currentFun.cycleStack.Pop();
            parent.Inner.Add(w.Body);
        }

        void evalDoWhile(TokenStream toks, CodeBlock blk)
        {
            /*
                do {
                    printf("Hello!");
                } while (i < 0);

                _while_label_X:
                    printf("Hello!");
                    if(i<0) goto _while_label_X;
             */
            var dw_block = new CodeBlock(blk);
            int dw_counter = do_while_label_counter;
            ++do_while_label_counter;
            evalLabel("_do_while_label_" + dw_counter + ": ;", dw_block);
            currentFun.cycleStack.Push(new Tuple<string, string>("_do_while_label_cond_" + dw_counter, "_do_while_label_out_" + dw_counter));
            //currentFun.cycleStack.Push("_do_while_label_" + dw_counter );

            evalBlock(toks, dw_block, true);
            if (toks.Next() != localize("while"))
                throw new CompilerException(ExceptionType.WhileExpected, "'while' expected after 'do' statement", toks);
            if (toks.Next() != "(")
                throw new CompilerException(ExceptionType.Brace, "'(' expected after 'while' statement", toks);
            var cond = "";
            int brc = 1;
            while (true)
            {
                if (toks.Next() == ")")
                    --brc;
                else if (toks.ToString() == "(")
                        ++brc;
                if (brc == 0)
                    break;

                cond += toks.ToString();

            }
            //toks.Next();
            bool eos = isEosNext(toks);
            evalLabel("_do_while_label_cond_" + dw_counter + ": ;", dw_block);
            var ts = new TokenStream(localize("if") + "(!(" + cond + ")) { " + localize("goto") + " _do_while_label_" + dw_counter + "; }");
            ts.Next();
            evalIf(ts, dw_block);
            evalLabel("_do_while_label_out_" + dw_counter + ": ;", dw_block);

            if (toks.ToString() == localize("else"))
            {
                if (eos)
                    throw new CompilerException(ExceptionType.BadExpression, "`else` without `if/for/while/do while`", toks);
                evalSingleStatement(toks, dw_block, true);
            }
            else if (!eos)
                    throw new CompilerException(ExceptionType.EosExpexted, "; expected", toks);

            evalLabel("_do_while_label_out_" + dw_counter + "_2: ;", dw_block);


            blk.Inner.Add(dw_block);
            currentFun.cycleStack.Pop();
        }

        private void evalBreak(TokenStream toks, CodeBlock parent)
        {
            checkEosNext(toks);
            string l = currentFun.cycleStack.Peek().Item2;
            if(l.StartsWith("_switch_"))    
                evalGoto(localize("goto") + " " + l + ";", parent);
            else
                evalGoto(localize("goto") + " " + l + "_2;", parent);
        }

        private void evalContinue(TokenStream toks, CodeBlock parent)
        {
            checkEosNext(toks);
            string l = currentFun.cycleStack.Peek().Item1;
            if(l != "")
                evalGoto(localize("goto") + " " + l + ";", parent);
        }

        private void evalReturn(TokenStream toks, CodeBlock parent)
        {
            Return ret = new Return();
            string str = "";
            //toks.Next();
            while (!isEosNext(toks))
            {
                str += toks.ToString() + " ";
            }

            if (str != "")
                ret.Statement = evalExpr(str, parent);
            else
                ret.noData = true;
            parent.Inner.Add(ret);
        }

        private void evalIf(TokenStream toks, CodeBlock parent)
        {
            if (toks.Next() != "(")
                throw new CompilerException(ExceptionType.Brace, "Brace '(' expected after 'if' statement", toks);
            string cond = "";
            int brc = 1;
            bool containsElseIfs = false;
            toks.Next();
            while (true)
            {
                if (toks.ToString() == ")")
                    --brc;
                else if (toks.ToString() == "(")
                        ++brc;
                if (brc == 0)
                    break;

                cond += toks.ToString() + " ";
                toks.Next();
            }
            If _if = new If(new CodeBlock(parent), null);
            _if.Cond.Inner.Add(evalExpr(cond, _if.Cond));
            _if.Body = evalSingleStatement(toks, parent);
            toks.PushBack();
            /*if (toks.Next() == "{")
            {
                toks.PushBack();
                _if.Body = evalBlock(toks, parent);
                //toks.Next();
            }
            else
            {
                string str = "";
                while (!isEos(toks))
                {
                    str += toks.ToString() + " ";
                    //toks.Next();
                }
                _if.Body = evalExpr(str, parent);
            }*/
            if (toks.Next() != localize("else"))
            {
                //toks.PushBack();
                _if.Label = "_if_out_label_" + if_label_counter;
                parent.Inner.Add(_if); 
                evalLabel("_if_out_label_" + if_label_counter + ": ;", parent);
                ++if_label_counter;
                return;
            }
            int if_ptr;
            _if.Label = "_if_else_label_" + if_label_counter;
            parent.Inner.Add(_if); 
            evalLabel("_if_else_label_" + if_label_counter + ": ;", parent);
            ++if_label_counter;
            int if_counter = if_label_counter;
            ++if_label_counter;
            if_ptr = parent.Inner.Count - 2;

            if (toks.Next() == localize("if"))
            {
                evalGoto(localize("goto") + " _if_out_label_" + if_counter + ";", (parent.Inner[if_ptr] as If).Body);
                containsElseIfs = true;
                var elif = new If(null, null);
                do
                {
                    elif = evalElseIf(toks, parent, (parent.Inner[if_ptr] as If));
                    evalGoto(localize("goto") + " _if_out_label_" + if_counter + ";", elif.Body);
                    elif.Label = "_if_else_label_" + if_label_counter;
                    parent.Inner.Add(elif);
                    evalLabel("_if_else_label_" + if_label_counter + ": ;", parent);
                    ++if_label_counter;
                } while (checkElseIf(toks));
                //toks.Next();
                if (toks.Next() == localize("else"))
                    evalElse((parent.Inner[if_ptr] as If), toks, parent);
                else
                {
                    parent.Inner.Remove(parent.Inner.Last());
                    var lastElIf = parent.Inner.Last() as If;
                    lastElIf.Body.Inner.Remove(elif.Body.Inner.Last());
                    lastElIf.Label = "_if_out_label_" + if_counter;
                }
            }
            else //if (toks.ToString() == "else")
            {
                toks.PushBack();
                evalElse((parent.Inner[if_ptr] as If), toks, parent);

                evalGoto(localize("goto") + " _if_out_label_" + if_counter + ";", (parent.Inner[if_ptr] as If).Body);
            }
            if ((parent.Inner[if_ptr] as If).Else != null)
                parent.Inner.Add((parent.Inner[if_ptr] as If).Else);
            evalLabel("_if_out_label_" + if_counter + ": ;", parent);
        }

        private void evalElse(If @if, TokenStream toks, CodeBlock parent)
        {
            @if.Else = evalSingleStatement(toks, parent);
        }

        private bool checkElseIf(TokenStream toks)
        {
            int pos = toks.Position;
            if (toks.Next() != localize("else"))
            {
                toks.PushBack();
                return false;
            }
            if (toks.Next() == localize("if"))
                return true;
            toks.Position = pos;
            return false;
        }

        private If evalElseIf(TokenStream toks, CodeBlock parent, If _if)
        {
            if (toks.Next() != "(")
                throw new CompilerException(ExceptionType.Brace, "Brace '(' expected after 'if' statement", toks);
            string cond = "";
            int brc = 1;
            toks.Next();
            while (true)
            {
                if (toks.ToString() == ")")
                    --brc;
                else if (toks.ToString() == "(")
                        ++brc;
                if (brc == 0)
                    break;

                cond += toks.ToString() + " ";
                toks.Next();
            }
            If elif = new If(new CodeBlock(parent), null);
            elif.Cond.Inner.Add(evalExpr(cond, elif.Cond));
            elif.Body = evalSingleStatement(toks, parent);
            toks.PushBack();
            //toks.Next();
            /*if (toks.Next() == "{")
            {
                toks.PushBack();
                elif.Body = evalBlock(toks, parent);
            }
            else
            {
                string str = "";
                while (!isEos(toks))
                {
                    str += toks.ToString() + " ";
                    toks.Next();
                }
                elif.Body = evalExpr(str, parent);
            }*/
            return elif; 
        }

        void evalSwitch(TokenStream toks, CodeBlock blk)
        {
            Switch sw = new Switch(new CodeBlock(blk), new CodeBlock(blk), new CodeBlock(blk));

            if (toks.Next() != "(")
                throw new CompilerException(ExceptionType.Brace, "Brace '(' expected after 'switch' statement", toks);
            var cond = "";
            int brc = 1;
            toks.Next();
            while (true)
            {
                if (toks.ToString() == ")")
                    --brc;
                else if (toks.ToString() == "(")
                        ++brc;
                if (brc == 0)
                    break;

                cond += toks.ToString();
                toks.Next();
            }
            string src = "";
            sw.Cond.Inner.Add(evalExpr(cond, sw.Cond));
            var bp = (BasicPrimitive)sw.Cond.Inner[0];
            if (bp.Polish.Count != 1)
            {
                string type = bp.Type.ToString().ToLower();
                var v = src = "t" + (++currentFun.tempVarCounter);
                sw.Cond.Inner.Clear();
                evalLocalVar(type, new TokenStream(v + " = " + cond + ";"), sw.Cond);
                evalExpr(v, sw.Cond);
            }
            else
                src = cond.Trim();
            if (toks.Next() != "{")
                throw new CompilerException(ExceptionType.Brace, "`{` expected after `switch` statement", toks);
            toks.Next();
            bool defaultExists = false;
            string sw_end = "_switch_end_" + switch_label_counter++;
            string sw_def = "_switch_def_" + switch_label_counter;

            List<string> consts = new List<string>();
            
            while (toks.ToString() != "}")
            {
                if (toks.ToString() == "case")
                {
                    // TODO: OPTIMIZE
                    /*string const_val = "";
                    while (toks.Next() != ":")
                    {f (toks.ToString() == ")")
                            --brc;
                        else if (toks.ToString() == "(")
                            ++brc;
                        if (brc == 0)
                            break;

                        const_val += toks.ToString() + " ";
                        //toks.Next();
                    }*/
                    toks.Next();
                    while (true)
                    {
                        string const_val = toks.ToString();
                        if (consts.Contains(const_val))
                            throw new CompilerException(ExceptionType.DuplicateValue, "two same constants `" + const_val + "` in `switch`", toks);
                        consts.Add(const_val);
                        var exp = evalExpr(const_val, new CodeBlock());
                        var tmpt = new TokType[]{ TokType.Boolean, TokType.Int, TokType.Double, TokType.String };
                        if (!tmpt.Contains(exp.Polish[0].Type))
                            throw new CompilerException(ExceptionType.IllegalType, "Illegal type in `case`", toks);
                        //if (toks.Next() != ":")
                        //    throw new CompilerException(ExceptionType.DoubledotExpected, "`:` expected after `case` and condition", toks);
                        var ts = new TokenStream("if (!(" + src + " == " + const_val + ")) { goto " + "_switch_label_" + switch_label_counter++ + "; }");
                        ts.Next();
                        evalIf(ts, sw.Jumps);
                        evalLabel("_switch_label_" + (switch_label_counter - 1) + ":", sw.Body);

                        if (toks.Next() == ":")
                            break;
                        else if (toks.ToString() == "|")
                                toks.Next();
                        else
                            throw new CompilerException(ExceptionType.DoubledotExpected, "`:` or `|` expected after `case` and condition", toks);
                    } 

                    currentFun.cycleStack.Push(new Tuple<string, string>("", sw_end));
                    while (toks.ToString() != "case" && toks.ToString() != "default" && toks.ToString() != "}")
                    {
                        evalSingleStatement(toks, sw.Body, true, true);
                        toks.PushBack();
                    }
                    toks.Next();
                    currentFun.cycleStack.Pop();
                }
                else if (toks.ToString() == "default")
                {
                    if (toks.Next() != ":")
                        throw new CompilerException(ExceptionType.DoubledotExpected, "`:` expected after `default`", toks);
                    evalLabel(sw_def + ":", sw.Body);
                    defaultExists = true;
                    currentFun.cycleStack.Push(new Tuple<string, string>("", sw_end));
                    while (toks.ToString() != "}")
                    {
                        evalSingleStatement(toks, sw.Body, true, true);
                        toks.PushBack();
                    }
                    toks.Next();
                    currentFun.cycleStack.Pop();
                }
            }
            if (defaultExists)
            {
                evalGoto("goto " + sw_def + ";", sw.Jumps);
            }
            evalLabel(sw_end + ":", sw.Body);
            blk.Inner.Add(sw);
        }

        void evalLabel(string str, CodeBlock blk)
        {
            var ts = new TokenStream(str);
            ts.Next();
            evalLabel(ts, blk);
        }

        void evalLabel(TokenStream ts, CodeBlock blk)
        {
            var lbl = getIdentifier(ts);
            if (ts.Next() != ":")
                throw new Exception(": expected!");
            Label l = new Label();
            l.StrLabel = lbl;
            blk.Inner.Add(l);
            ts.Next();
        }

        private void evalFor(TokenStream toks, CodeBlock parent)
        {
            For @for = new For();
            CodeBlock forBlock = new CodeBlock(parent);
            if (toks.Next() != "(")
                throw new CompilerException(ExceptionType.Brace, "Brace '(' expected after 'if' statement", toks);
            string type = null, ident = getIdentifierNext(toks);
            toks.Next();
            if (toks.Type == TokenType.Identifier)
            {
                type = ident;
                ident = toks.ToString();
                toks.PushBack();
                //evalLocalVar(type, toks, @for.Init, @for.Decls, forBlock);
                evalLocalVar(type, toks, forBlock);
                @for.Init = forBlock.Inner.ToList();
                @for.Decls = forBlock.Locals.ToList();
                //int i = 0, b = 2; //for (toks.Next(), b = 3, evalExpr(""); i < 10 && b > 0; i++, b++, toks.Next(), Console.WriteLine("")) ;
            }
            else
            {
                //toks.PushBack();
                string cur = ident + " ";
                int brc = 0;
                //toks.Next();
                while (toks.ToString() != ";" && toks.Type != TokenType.Separator)
                {
                    if (brc == 0 && toks.ToString() == "," && toks.Type == TokenType.Delimiter)
                    {
                        var expr = evalExpr(cur, forBlock);
                        @for.Init.Add(expr);
                        forBlock.Inner.Add(expr);
                        cur = "";
                        toks.Next();
                        continue;
                    }
                    else if (toks.ToString() == "(")
                            ++brc;
                        else if (toks.ToString() == ")")
                                --brc;
                    cur += toks.ToString() + " ";
                    toks.Next();
                }
            }
            var for_in = "_for_label_" + for_label_counter;
            var for_out = "_end_for_label_" + for_label_counter;
            currentFun.cycleStack.Push(new Tuple<string, string>(for_in, for_out));

            ++for_label_counter;
            evalLabel(for_in + ": ;", forBlock);

            string cond = "";
            while (toks.Next() != ";" && toks.Type != TokenType.Separator)
            {
                cond += toks.ToString() + " ";
            }
            //@for.Cond = evalExpr(cond, forBlock);

            cond = localize("if") + "(" + cond + ") { " + localize("goto") + " " + for_out + " ; }";
            TokenStream condts = new TokenStream(ref cond);
            condts.Next();
            evalIf(condts, forBlock);

            toks.Next();
            string exp = "";
            int brccount = 0;
            List<BasicPrimitive> counters = new List<BasicPrimitive>();
            while (true)
            {
                if (brccount == 0 && toks.ToString() == "," && toks.Type == TokenType.Delimiter)
                {
                    var bp = evalExpr(exp, forBlock);
                    counters.Add(bp);
                    @for.Counter.Add(bp);
                    exp = "";
                    toks.Next();
                    continue;
                }
                else if (toks.ToString() == "(")
                        ++brccount;
                    else if (toks.ToString() == ")")
                        {
                            --brccount;
                            if (brccount == -1)
                            {
                                var bp = evalExpr(exp, forBlock);
                                counters.Add(bp);
                                @for.Counter.Add(bp);
                                break;
                            }
                        }
                exp += toks.ToString() + " ";
                toks.Next();
            }
            evalSingleStatement(toks, forBlock, true);

            /*if (toks.Next() == "{")
            {
                toks.PushBack();
                @for.Body = evalBlock(toks, forBlock, true);

                toks.Next();
            }
            else //FIXME: Unhappy behaviour! :(
            {
                string e = "";
                while (!isEos(toks))
                {
                    e += toks.ToString() + " ";
                    toks.Next();
                }
                @for.Body = evalExpr(e, forBlock);
            }*/

            forBlock.Inner.AddRange(counters);

            evalGoto(localize("goto") + " " + for_in + ";", forBlock);

            evalLabel(for_out + ": ;", forBlock);

            if (toks.ToString() == localize("else"))
                evalSingleStatement(toks, forBlock, true);
            //else
            //    toks.PushBack();

            evalLabel(for_out + "_2: ;", forBlock);

            @for.forBlock = forBlock;
            parent.Inner.Add(@for.forBlock);
            currentFun.cycleStack.Pop();
            //parent.Inner.Add (forBlock);
        }

        private void evalDeclaration(DataTypes t, string sname = "")
        {
            string ident = tokens.Next();

            bool isArray = false;
            int dimens = 1;
            DataType dt;

            if (ident == "[")
            {
                isArray = true;
                if (tokens.Next() != "]")
                    throw new CompilerException(ExceptionType.Brace, "`]` expected in array declaration", tokens);
                while (true)
                {
                    if (tokens.Next() == "[")
                        dimens++;
                    else
                    {
                        //tokens.PushBack();
                        ident = tokens.ToString();
                        dt = new DataType(DataTypes.Array, new DataType(t), dimens);
                        break;
                    }
                    if (tokens.Next() != "]")
                        throw new CompilerException(ExceptionType.Brace, "`]` expected in array declaration", tokens);
                }
                ident = tokens.ToString();
            }
            else
            {
                //tokens.PushBack();
                dt = new DataType(t);
            }

            if (tokens.Next() == "(")
            {
                evalFunction(ident, dt, sname); // int main(string args)
            }
            else
            {
                //tokens.PushBack(); 
                evalGlobalVar(ident, dt, sname); // public double PI = 3.14;
            }
            isPublic = isNative = false;
        }

        //InDev, will be in next release, maybe
        private void evalNamespace()
        {
            throw new NotImplementedException();
        }

        private void evalImport()
        {
            var mod = tokens.Next();
            if (tokens.Type != TokenType.String)
                throw new CompilerException(ExceptionType.Import, "Import fault: " + tokens.ToString(), tokens);
            if (mod == Module.VM_INTERNAL)
                return;
            mod = mod.Replace("\"", "");

            dirs.Add(new Import(tokens.ToString()));
            Module im = new Module(this);
            im.Load(mod);
            imported.Add(im);
            //dirs.Add(new Import(mod));
        }

        //InDev, will be in next release
        private void evalUsing()
        {
            throw new NotImplementedException();
        }

        //InDev, will be in next major release
        private void evalStruct()
        {
            throw new NotImplementedException();
        }

        private void evalNative()
        {
            throw new NotImplementedException();
        }

        //InDev, will be in next release
        private void evalAttribute()
        {
            AttributeReader reader = new AttributeReader(tokens);

            var attr = reader.Read();

            switch (attr.Name)
            {
                case "Locale":
                    if (attr.Data.Count == 1 && attr.Data[0].Value is string)
                    {
                        string loc = (attr.Data[0].Value as string).ToLower();
                        if (loc == "english" || loc == "англійська")
                            CommandArgs.lang = SyntaxLang.English;
                        else if (loc == "ukrainian" || loc == "українська")
                                CommandArgs.lang = SyntaxLang.Ukrainian;
                            else
                                throw new CompilerException(ExceptionType.UnknownLocale, "Unknown locale " + loc, tokens);
                    }
                    else
                    {
                        throw new AttributeException("Locale", "Incorrect data");
                    }
                    break;
                case "AddMeta":
                case "Info":

                    if ((attr.Data.Count > 2 || attr.Data.Count < 1 || !(attr.Data[0].Value is string)) && !attr.Data[0].IsOptional)
                    {
                        throw new AttributeException("AddMeta", "Incorrect data");
                    }

                    Metadata md = new Metadata();
                    if (attr.Data[0].IsOptional)
                    {
                        md.key = attr.Data[0].Key;
                        md.value = attr.Data[0].Value;
                        md.type = attr.Data[0].Type;
                    }
                    else
                    {
                        md.key = attr.Data[0].Value as string;
                        if (attr.Data.Count == 2)
                        {
                            md.value = attr.Data[1].Value;
                            md.type = attr.Data[1].Type;
                        }
                        else
                            md.type = DataTypes.Void;
                    }
                    var mlist = from m in meta
                                               where m.key == md.key
                                               select m;
                    if (mlist.ToList().Count != 0)
                        throw new CompilerException(ExceptionType.MetaKeyExists, "Meta key " + md.key + " in module " + CommandArgs.source + " exists", tokens);

                    meta.Add(md);
                    break;
                case "Module":
                    if (attr.Data.Count != 1 && !(attr.Data[0].Value is string))
                        throw new AttributeException("Module", "Incorrect module name");
                    if (CommandArgs.outFile == null)
                        CommandArgs.outFile = _out = (attr.Data[0].Value as string) + ".vas";
                    break;
                case "RuntimeInternal":
                    if(attr.Data.Count != 0)
                        throw new AttributeException("RuntimeInternal", "Too many arguments");
                    if (!attr.Binded)
                        throw new AttributeException("RuntimeInternal", "`@RuntimeInternal` must be binded to function (check `;`)");
                    bindedAttributes.Add(attr);
                    break;
                case "Entry":
                    if (attr.Data.Count != 0)
                        throw new AttributeException("Entry", "Too many arguments");
                    if (!attr.Binded)
                    throw new AttributeException("Entry", "`@Entry` must be binded to function (check `;`)");
                    bindedAttributes.Add(attr);
                    break;
                default:
                    break;
            }

            /*var attrIdent = tokens.Next();
            switch (attrIdent)
            {
                case "Locale":
                    if (tokens.Next() != "(")
                        throw new CompilerException(ExceptionType.Brace, "Brace '(' expected", tokens);
                    var loc = tokens.Next().ToLower().Replace("\"", "");
                    //if(new String [] {"english", "ukrainian", "англійська", "українська"}.Contains(loc.Replace("\"", ""));
                    if (loc == "english" || loc == "англійська")
                        CommandArgs.lang = SyntaxLang.English;
                    else if (loc == "ukrainian" || loc == "українська")
                        CommandArgs.lang = SyntaxLang.Ukrainian;
                    else
                        throw new CompilerException(ExceptionType.UnknownLocale, "Unknown locale " + loc, tokens);
                    if (tokens.Next() != ")")
                        throw new CompilerException(ExceptionType.Brace, "Brace ')' expected", tokens);
                    if(!isEosNext(tokens))
                        throw new CompilerException(ExceptionType.EosExpexted, "End of statement expected (;)", tokens);
                    break;

                case "AddMeta":
                    if (tokens.Next() != "(")
                        throw new CompilerException(ExceptionType.Brace, "Brace '(' expected", tokens);
                    Metadata md = new Metadata();
                    md.key = tokens.Next().Replace("\"", "");
                    var mlist = from m in meta
                                               where m.key == md.key
                                               select m;
                    if (mlist.ToList().Count != 0)
                        throw new CompilerException(ExceptionType.MetaKeyExists, "Meta key " + md.key + " in module " + CommandArgs.source + " exists", tokens);
                    if (tokens.Next() == ")")
                        md.type = DataTypes.Void;
                    else
                    {
                        if (tokens.ToString() != ",")
                            throw new CompilerException(ExceptionType.ComaExpected, "Coma expected", tokens);
                        var val = tokens.Next();
                        switch (tokens.Type)
                        {
                            case TokenType.Boolean:
                                md.value = bool.Parse(val);
                                md.type = DataTypes.Bool;
                                break;
                            case TokenType.String:
                                md.value = val.Remove(0, 1).Remove(val.Length - 2, 1);
                                md.type = DataTypes.String;
                                break;
                            case TokenType.Digit:
                                if (val.Contains("."))
                                {
                                    md.value = double.Parse(val, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture);
                                    md.type = DataTypes.Double;
                                }
                                else
                                {
                                    md.value = int.Parse(val);
                                    md.type = DataTypes.Int;
                                }
                                break;
                            default:
                                throw new CompilerException(ExceptionType.IllegalType, "Unsupported/unknown type of meta value", tokens);
                                break;
                        }
                    }
                    meta.Add(md);
                    break;
                case "Module":
                    break;
                default:
                    break;
            }
            */
        }

        private void evalFunction(string ident, DataType t, string sname)
        {
            Function f;
            if (t.type != DataTypes.User)
                f = new Function(ident, t);
            else
                f = new Function(ident, t, new StructType(""));

            f.isNative = isNative;
            f.isPublic = isPublic;

            if (bindedAttributes.Count != 0)
            {
                foreach (var attr in bindedAttributes)
                {
                    Console.WriteLine("Parsing binded attribute " + attr.Name);
                    switch (attr.Name)
                    {
                        case "RuntimeInternal":
                            f.flags |= Function.F_RTINTERNAL;
                            break;
                        default:
                            break;
                    }
                }
                bindedAttributes.Clear();
            }

            if (tokens.ToString() != "(")
            {
                throw new CompilerException(ExceptionType.Brace, "'(' expected", tokens);
            }
            while (true)
            {
                if (tokens.Next() == ")")
                    break;

                var type = getIdentifier(tokens);
                VarType v;
                if (isPODType(type))
                    v = new VarType(VarType.ARG_PREFIX + getIdentifierNext(tokens), getPODType(type));
                else
                    throw new NotImplementedException();
                //v = new VarType(VarType.ARG_PREFIX + getIdentifierNext(tokens), DataTypes.User, new StructType(type));
                if (tokens.Next() != "," && tokens.ToString() != ")")
                    throw new CompilerException(ExceptionType.ComaExpected, "',' (coma) expected", tokens);
                f.argTypes.Add(v);
                if (tokens.ToString() == ")")
                    break;
            }

            if (funcs.Any(fun => fun == f) || imported.Any(m => m.functions.Any(fun => fun == f)))
                throw new CompilerException(ExceptionType.FunctionRedefinition, "Function " + f.name + " with args (" + f.ArgsFromList() + ") already defined", tokens);

            if (tokens.Next() != "{")
            {
                if ((f.flags & Function.F_RTINTERNAL) != 0)
                {
                    if (tokens.ToString() != ";")
                        throw new CompilerException(ExceptionType.IllegalToken, "`;` expected", tokens);
                    funcs.Add(f);
                    return;

                }
                else
                    throw new CompilerException(ExceptionType.Brace, "`{` expected", tokens);
            }
            int braceCount = 1;
            f.body = "{";
            while (braceCount != 0)
            {
                string toks = tokens.Next();

                if (toks == "{")
                    ++braceCount;
                else if (toks == "}")
                        --braceCount;

                if (braceCount != 0)
                    f.body += toks + " ";
            }
            f.body += "}";
            funcs.Add(f);
        }

        private string getIdentifier(TokenStream toks)
        {
            var id = toks.ToString();
            if (toks.Type != TokenType.Identifier)
                throw new CompilerException(ExceptionType.IllegalToken, "Identifier expected", toks);
            return id;
        }

        private string getIdentifierNext(TokenStream toks)
        {
            var id = toks.Next();
            if (toks.Type != TokenType.Identifier)
                throw new CompilerException(ExceptionType.IllegalToken, "Identifier expected", toks);
            return id;
        }

        private DataTypes getPODType(string str)
        {
            string type = unlocalize(str);
            object ret = Enum.Parse(typeof(DataTypes), type[0].ToString().ToUpper() + type.Remove(0, 1));
            if (ret == null)
                return DataTypes.User;
            return (DataTypes)ret;
            /*switch (str)
            {
                case "bool":
                    return DataTypes.Bool;
                case "byte":
                    return DataTypes.Byte;
                case "short":
                    return DataTypes.Short;
                case "int":
                    return DataTypes.Int;
                case "uint":
                    return DataTypes.Uint;
                case "long":
                    return DataTypes.Long;
                case "ulong":
                    return DataTypes.Ulong;
                case "string":
                    return DataTypes.String;
                case "double":
                    return DataTypes.Double;
                case "void":
                    return DataTypes.Void;
                default:
                    return DataTypes.User;
            }*/
        }

        private void checkEos(TokenStream toks)
        {
            if (!isEos(toks))
                throw new CompilerException(ExceptionType.EosExpexted, "End of statement expected (;)", tokens);
        }

        private void checkEosNext(TokenStream toks)
        {
            if (!isEosNext(toks))
                throw new CompilerException(ExceptionType.EosExpexted, "End of statement expected (;)", tokens);
        }

        private bool isEos(TokenStream toks)
        {
            if (toks.ToString() != ";" && toks.Type != TokenType.Separator)
                return false;
            return true;
        }

        private bool isEosNext(TokenStream toks)
        {
            if (toks.Next() != ";" && toks.Type != TokenType.Separator)
                return false;
            return true;
        }

        private bool isPODType(string str, bool @void = false)
        {
            if (CommandArgs.lang == SyntaxLang.English)
                switch (str)
                {
                    case "bool":
                    case "byte":
                    case "short":
                    case "int":
                    case "uint":
                    case "long":
                    case "ulong":
                    case "string":
                    case "double":
                        return true;
                    case "void":
                        if (@void)
                            return true;
                        else
                            return false;
                    default:
                        return false;
                }
            else
                switch (str)
                {
                    case "буль":
                    case "байт":
                    case "зкоротке":
                    case "зціле":
                    case "ціле":
                    case "здовге":
                    case "довге":
                    case "рядок":
                    case "подвійне":
                        return true;
                    case "воід":
                        if (@void)
                            return true;
                        else
                            return false;
                    default:
                        return false;
                }
        }

        private string unlocalize(string src)
        {
            if (CommandArgs.lang == SyntaxLang.English)
                return src;
            else if (CommandArgs.lang == SyntaxLang.Ukrainian)
                {
                    switch (src)
                    {
                        case "якщо":
                            return "if";
                        case "інакше":
                            return "else";
                        case "для":
                            return "for";
                        case "робити":
                            return "do";
                        case "поки":
                            return "while";
                        case "повернути":
                            return "return";
                        case "зупинити":
                            return "break";
                        case "продовжити":
                            return "continue";
                        case "перейти":
                            return "goto";
                        case "публічний":
                        case "публічна":
                            return "public";
                        case "приватний":
                        case "приватна":
                            return "private";
                        case "імпорт":
                            return "import";
                        case "буль":
                            return "bool";
                        case "байт":
                            return "byte";
                        case "зкоротке":
                            return "short";
                        case "зціле":
                            return "int";
                        case "ціле":
                            return "uint";
                        case "здовге":
                            return "long";
                        case "довге":
                            return "ulong";
                        case "рядок":
                            return "string";
                        case "подвійне":
                            return "double";
                        case "воід":
                            return "void";
                        default:
                            break;
                    }
                }
            return src;
        }

        private string localize(string src)
        {
            if (CommandArgs.lang == SyntaxLang.English)
                return src;
            else if (CommandArgs.lang == SyntaxLang.Ukrainian)
                {
                    switch (src)
                    {
                        case "if":
                            return "якщо";
                        case "else":
                            return "інакше";
                        case "for":
                            return "для";
                        case "do":
                            return "робити";
                        case "while":
                            return "поки";
                        case "return":
                            return "повернути";
                        case "break":
                            return "зупинити";
                        case "continue":
                            return "продовжити";
                        case "goto":
                            return "перейти";
                        case "public":
                            return "публічний";
                        case "private":
                            return "приватний";
                        case "import":
                            return "імпорт";
                        case "bool":
                            return "буль";
                        case "byte":
                            return "байт";
                        case "short":
                            return "зкоротке";
                        case "int":
                            return "зціле";
                        case "uint":
                            return "ціле";
                        case "long":
                            return "здовге";
                        case "ulong":
                            return "довге";
                        case "string":
                            return "рядок";
                        case "double":
                            return "подвійне";
                        case "void":
                            return "воід";
                        default:
                            break;
                    }
                }
            return src;
        }
    }
}
