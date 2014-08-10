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

namespace Translator
{
    public class Compiler
    {
        string code;
        string vasm;

        List<object> dirs = new List<object>();

        TokenStream tokens;
        List<Function> funcs = new List<Function>();
        List<Function> imported = new List<Function>();
        CodeBlock globalVars = new CodeBlock();
        OpPriority priorities = new OpPriority();
        //ArrayList imports = new ArrayList();
        Function currentFun = null;
        int brace_counter = 0, for_label_counter = 0, while_label_counter = 0, do_while_label_counter = 0, if_label_counter = 0;
        bool isPublic = false, isNative = false;
        string _in;
        string _out;

        public static Compiler GetTestClass()
        {
            return new Compiler();
        }

        private Compiler() // Demo
        {
            funcs.Add(new Function("fun", DataTypes.Null));
            funcs.Add(new Function("funA", DataTypes.Null));
            funcs.Add(new Function("funB", DataTypes.Null));
            funcs.Add(new Function("exp", DataTypes.Null));
            funcs.Add(new Function("func", DataTypes.Null));
        }

        public Compiler(string _in, string _out)
        {
            imported.Add(new Function("::vm.internal", "print", DataTypes.Void, new List<VarType> {new VarType(DataTypes.String)}));
            imported.Add(new Function("::vm.internal", "read", DataTypes.String, new List<VarType> {}));
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
            priorities.Add("#!", "#~", "#+", "#-");
            priorities.Add("++", "--", ".");
            priorities.Add("");
            funcs.Add(new Function("__global_constructor__", DataTypes.Void));

            this._in = _in;
            this._out = _out;
            tokens = new TokenStream(new StreamReader(_in).ReadToEnd());
        }

        public void Compile()
        {
            compileGlobal();
            //Console.ReadKey();
            foreach (var fun in funcs)
            {
                currentFun = fun;
                fun.mainBlock = compileBlock(new TokenStream(ref fun.body), globalVars);
            }
            ILCompiler ilc = new ILCompiler(dirs, funcs, globalVars, _out);
            ilc.Compile();

            //return;
            StreamWriter sw = new StreamWriter(_out+".tac");
			
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
                //	sw.WriteLine (" = " + g.val.ToString ());
                //else
                //	sw.WriteLine ("");
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

            return;
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
                    //	sw.WriteLine (new String (' ', offset + 4) + (_if.Body as BasicPrimitive).Expr);
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
                        //	sw.WriteLine (new String (' ', offset + 4) + (elif.Body as BasicPrimitive).Expr);
                    }
                    //if (_if.Else != null) {
                    //	sw.WriteLine(new String (' ', offset) + "else ");
                    //	if (_if.Else is CodeBlock)
                    //		writeBlock (_if.Else as CodeBlock, sw, offset);
                    //	else 
                    //		sw.WriteLine (new String (' ', offset + 4) + (_if.Else as BasicPrimitive).Expr);
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
                else if (obj is Return)
                {
                    Return r = obj as Return;
                    sw.WriteLine(new String(' ', offset) + "return " + r.Statement.Expr);
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
                        evalAtributte();
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
                    default:
                        if (tokens.Type == TokenType.Identifier)
                            evalDeclaration(DataTypes.User, tokens.ToString());
                        break;
                }
            }
            
            funcs[0].body += "}";
        }

        internal CodeBlock compileBlock(TokenStream toks, CodeBlock parent, bool isCurrent=false)
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

        internal BasicPrimitive evalExpr(string str, CodeBlock blk)
        {
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

            var stack = new Stack<Token>();
            TokenStream infix = new TokenStream(ref str);

            var polish = new List<Token>();
            
            infix.Next();

            bool isUnary = true, isFun = false;
            int fcount = 0;
            while (infix.Type != TokenType.EOF)
            {
                if (infix.Type == TokenType.Digit) // TODO: add other types (ui8, i16...) 
                {
                    if (infix.ToString().Contains('.'))
                        polish.Add(new Token(infix.ToString(), TokType.Double, DataTypes.Double));
                    else
                        polish.Add(new Token(infix.ToString(), TokType.Int, DataTypes.Int));
                    isUnary = false;
                }
                else if (infix.Type == TokenType.Identifier)
                {
                    bool isf = false;
                    //Function func = null;
                    if (funcs.Any(f => f.name == infix.ToString()) || imported.Any(f => f.name == infix.ToString()))
                    {
                        isf = isFun = true;
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

                        stack.Push(new Token (newname, TokType.Function));
                    }
                    else
                    {
                        var varName = infix.ToString();
                        DataTypes varType = DataTypes.Null;
                        CodeBlock b = blk.Parent;
                        int level = 0;
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
                                        varType = vd.Var.type;
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
                                        varType = currentFun.argTypes[i].type;
                                    }
                                }
                                for (int i = 0; i < globalVars.Locals.Count; ++i)
                                {
                                    if (globalVars.Locals[i].Var.name == VarType.FIELD_PREFIX + varName)
                                    {
                                        level = 0;
                                        idx = i;
                                        varType = globalVars.Locals[i].Var.type;
                                    }
                                }
                                if (level == -1)
                                    throw new Exception("Unknown var " + varName);
                            }

                            if (isArg)
                                varName = VarType.ARG_PREFIX + idx;
                            else if (level == 0)
                                varName = VarType.FIELD_PREFIX + idx;
                            else
                                varName = VarType.LOCAL_PREFIX + idx;
                        }
                        polish.Add(new Token (varName, TokType.Variable, varType));
                    }
                    isUnary = false;
                }
                else if (infix.ToString() == "(")
                {
                    stack.Push(new Token (infix.ToString(), TokType.OpenBrc));
                    isUnary = true;
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
                        if(stack.Count == 0)
                            throw new CompilerException(ExceptionType.BadExpression, "Bad expression: " + str, null);
                        polish.Add(pop);
                        pop = stack.Pop();
                    }
                    //if (stack.Count == 0)

                    if (stack.Count != 0 && stack.Peek().Type == TokType.Function)
                        polish.Add(stack.Pop());
                    isUnary = false;
                }
                else if (infix.Type == TokenType.Operator || infix.Type == TokenType.OperatorEq)
                {
                    while (stack.Count != 0 && checkTopOp(infix.ToString(), stack.Peek(), isUnary))
                    {
                        polish.Add(stack.Pop());
                    }
                    if (isUnary)
                        stack.Push(new Token ("#" + infix.ToString(), TokType.Operator));
                    else
                        stack.Push(new Token (infix.ToString(), TokType.Operator));
                    isUnary = true;
                }
                else if (infix.Type == TokenType.Boolean)
                {
                    polish.Add(new Token (infix.ToString(), TokType.Boolean, DataTypes.Bool));
                    isUnary = false;
                }
                else if (infix.Type == TokenType.String)
                {
                    polish.Add(new Token (infix.ToString(), TokType.String, DataTypes.String));
                    isUnary = false;
                }
                infix.Next();
            }

            while (stack.Count != 0)
                polish.Add(stack.Pop());

            string res = polish.Aggregate("", (current, p) => current + (p.StringRep + " "));
            var bp = new BasicPrimitive(new DataType(DataTypes.Void), res.Trim(), polish);
            var tac = evalTAC(bp);
            for (int i = 0; i < tac.Count-1; ++i)
            {
                blk.Inner.Add(tac[i]);
            }
            return tac.Last();
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

                    for (int t = i-count; t < i; ++t)
                    {
                        argTypes.Add(getVarType(rpn[t]));
                    }

                    Function f = getFunctionByArgList(op.Remove(op.LastIndexOf("@")), argTypes);
                    var funName = f.name;
                    if (f.module != "")
                    {
                        funName = "["+f.module+"] " + f.name;
                    }
                    var taccode = new BasicPrimitive();
                    if (f.type != DataTypes.Void && rpn.Count != 1)
                    {
                        Token retval = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, f.type);
                        ++currentFun.tempVarCounter;

                        taccode.Expr = retval.StringRep + " = _@call@_ " + funName + " (";
                        //taccode.Type = retval.Type;
                        taccode.Polish.Add(retval);
                        taccode.Polish.Add(new Token ("=", TokType.OperatorEq));
                        taccode.mainOpIdx = 2;
                        taccode.Polish.Add(new Token ("_@call@_", TokType.Keyword));
                    }
                    else if (f.type != DataTypes.Void && rpn.Count == 1)
                    {
                        taccode.Expr = "_@call_noreturn@_" + funName + " (";
                        taccode.mainOpIdx = 0;
                        taccode.Polish.Add(new Token ("_@call_noreturn@_", TokType.Keyword));
                    }
                    else
                    {
                        taccode.Expr = "_@call@_ " + funName + " (";
                        taccode.mainOpIdx = 0;
                        taccode.Polish.Add(new Token ("_@call@_", TokType.Keyword));
                    }
                    //taccode.Polish.Add(new Token { StringRep = "_@call@_", Type = TokType.Keyword });
                    taccode.Polish.Add(new Token (funName + "("+f.ArgsFromList()+")", TokType.Function));					

                    foreach (var a in f.argTypes)
                    {
                        taccode.Expr += a.type.ToString().ToLower() + ", ";
                    }
                    if (f.argTypes.Count != 0)
                        taccode.Expr = taccode.Expr.Remove(taccode.Expr.Length - ", ".Length);
                    taccode.Expr += ") ";
                    for (int t = i-count, ac = 0; t < i; ++t, ++ac)
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
                    if (op.StartsWith("#") || op == "++" || op == "--")
                    {
                        Token tmp;
                        if (op == "#!")
                        {
                            tmp = new Token ("t"+currentFun.tempVarCounter, TokType.TempVar, DataTypes.Bool);
                            taccode.Polish.Add(tmp);
                            taccode.Polish.Add(new Token ("=", TokType.OperatorEq));
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
           
                            tmp = new Token ("t"+currentFun.tempVarCounter, TokType.TempVar, getVarType(rpn[i - 1]));
                            taccode.Expr = tmp.StringRep + " = " + op + " " + rpn[i - 1].StringRep;
                            //taccode.Type = getVarType(rpn[i - 1]);
                            taccode.Polish.Add(tmp);
                            taccode.Polish.Add(new Token ("=", TokType.OperatorEq));
                            taccode.Polish.Add(new Token (op, TokType.OperatorMono));
                            taccode.Polish.Add(rpn[i - 1]);
                            taccode.mainOpIdx = 2;
                            ++currentFun.tempVarCounter;
                        }
                        rpn.RemoveAt(i);
                        rpn.Insert(i, tmp);
                        rpn.RemoveAt(i - 1);
                    }
                    else if (new string [] { "==", "!=", "<", "<=", ">=", ">" }.Contains(op))
                    {
                        var res = new Token("t" + currentFun.tempVarCounter, TokType.TempVar, DataTypes.Bool);
                        taccode.Polish.Add(res);
                        taccode.Polish.Add(new Token("=", TokType.OperatorEq));
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
                        taccode.Polish.Add(new Token("=", TokType.OperatorEq));
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
                        taccode.Polish.Add(new Token("=", TokType.OperatorEq));
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
                        taccode.Polish.Add(new Token("=", TokType.OperatorEq));
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
                        assignment.Polish.Add(new Token("=", TokType.OperatorEq));
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

        Token convert(ref BasicPrimitive conv, Token from, DataTypes to, bool check=false)
        {
            //TEST:
            if (from.DType == to)
            {
                Console.WriteLine("DEBUG: type '" + from.DType.ToString() + "' == '" + to.ToString()+"'");
                return from;
            }

            if (check)
            {
                if (!canCastType(from.DType, to) && !canCastType(to, from.DType))
                    throw new CompilerException(ExceptionType.IllegalCast, "Illegal cast from " + from.DType.ToString() + " to " + to.ToString(), null);
            }
            var tmp = new Token ("t"+currentFun.tempVarCounter, TokType.TempVar, to);
            conv.Expr = "t" + currentFun.tempVarCounter + " = " + from.StringRep + " _@as@_ " + to.ToString().ToLower();
            conv.Polish.Add(tmp);
            conv.Polish.Add(new Token ("=", TokType.OperatorEq));
            conv.Polish.Add(from);
            conv.Polish.Add(new Token ("_@as@_", TokType.Operator));
            conv.Polish.Add(new Token (to.ToString().ToLower(), TokType.Keyword, to));
            conv.mainOpIdx = 3;
            ++currentFun.tempVarCounter;
            return tmp;
        }

        bool isNumeric(DataTypes dt)
        {
            return new List<DataTypes>
            {DataTypes.Byte, DataTypes.Short, DataTypes.Int, DataTypes.Uint, 
                DataTypes.Long, DataTypes.Ulong, DataTypes.Double
            }.Contains(dt);
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

        Function getFunctionByArgList(string name, List<DataTypes> argTypes)
        {
            List<Function> overloaded = new List<Function>();
            foreach (var f in funcs)
            {
                if (f.name == name && f.argTypes.Count == argTypes.Count)
                    overloaded.Add(f);
            }
            foreach (var f in imported)
            {
                if (f.name == name && f.argTypes.Count == argTypes.Count)
                    overloaded.Add(f);
            }
            if (overloaded.Count == 1)
                return overloaded[0];
            bool isEq = true;
            foreach (var f in overloaded)
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
                }
                if (isEq)
                    return f;
            }
            if (overloaded.Count == 1)
                return overloaded[0];
            else
                throw new Exception("Unexpected ambigious overload: " + name);
        }

        bool canCastType(DataTypes from, DataTypes to)
        {
            List<DataTypes> numeric = new List<DataTypes>
            {DataTypes.Byte, DataTypes.Short, DataTypes.Int, DataTypes.Uint, 
                DataTypes.Long, DataTypes.Ulong, DataTypes.Double
            };
            if (isNumeric(from) && isNumeric(to))
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
            if (isUnary)
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

        private CodeBlock evalSingleStatement(TokenStream toks, CodeBlock parent, bool current=false)
        {
            CodeBlock blk;
            if (current)
                blk = parent;
            else
                blk = new CodeBlock(parent);
            if (toks.Next() == ";" && toks.Type == TokenType.Separator)
            {
                blk.Inner.Add(null);
                return blk;
            }
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
                //string type = toks.Next();
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
                VarType vt = new VarType(ident, getPODType(type)); // Not supports STRUCTs
                VarDecl vd = new VarDecl();
                vd.Var = vt;
                vd.LocalIndex = currentFun.locals.Count;
                inner.Add(vd);
                vars.Add(vd);
                currentFun.locals.Add(currentFun.locals.Count, vt);
                var identId = VarType.LOCAL_PREFIX + vd.LocalIndex;
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
                        expr += toks.ToString();
                        if (toks.Type != TokenType.String)
                        {
                            if (toks.ToString() == "(")
                                ++brc;
                            else if (toks.ToString() == ")")
                                --brc;
                        }
                    }
                    inner.Add(evalExpr(expr, blk)); // Not supports STRUCTs
                }
                if (toks.ToString() == ",")
                    continue;
                break;
            }
        }

        private CodeBlock evalBlock(TokenStream toks, CodeBlock parent, bool isCurrent=false)
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
                    if(i == -1)
                        continue;
                }

                _in:
                if(!(i<0)) goto _out;
                    printf("Hello!");
                    if(i == -1) goto _in;
                    goto _in;
                _out:
             */
            if (toks.Next() != "(")
                throw new CompilerException(ExceptionType.Brace, "Brace '(' expected after 'if' statement", toks);
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

            cond = "if(!(" + cond + ")) { goto " + end_while + " ; }"; 
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
            var gts = new TokenStream("goto " + begin_while + ";");
            gts.Next();
            evalGoto(gts, body);
            
            evalLabel(end_while + ": ;", body);
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
            currentFun.cycleStack.Push(new Tuple<string, string>("_do_while_label_" + dw_counter, "_do_while_label_out_" + dw_counter));
            //currentFun.cycleStack.Push("_do_while_label_" + dw_counter );

            evalBlock(toks, dw_block, true);
            if (toks.Next() != "while")
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
            checkEosNext(toks);
            var ts = new TokenStream("if (" + cond + ") { goto _do_while_label_" + dw_counter + "; }");
            ts.Next();
            evalIf(ts, dw_block);
            evalLabel("_do_while_label_out_" + dw_counter + ": ;", dw_block);
            blk.Inner.Add(dw_block);
            currentFun.cycleStack.Pop();
        }
        
        private void evalBreak(TokenStream toks, CodeBlock parent) {
            checkEosNext(toks);
            evalGoto("goto " + currentFun.cycleStack.Peek().Item2 + ";", parent);
        }

        private void evalContinue(TokenStream toks, CodeBlock parent) {
            checkEosNext(toks);
            evalGoto("goto " + currentFun.cycleStack.Peek().Item1 + ";", parent);
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

            ret.Statement = evalExpr(str, parent);
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
            if (toks.Next() != "else")
            {
                toks.PushBack();
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

            if (toks.Next() == "if")
            {
                evalGoto("goto _if_out_label_" + if_counter + ";", (parent.Inner[if_ptr] as If).Body);
                containsElseIfs = true;
                var elif = new If(null, null);
                do
                {
                    elif = evalElseIf(toks, parent, (parent.Inner[if_ptr] as If));
                    evalGoto("goto _if_out_label_" + if_counter + ";", elif.Body);
                    elif.Label = "_if_else_label_" + if_label_counter;
                    parent.Inner.Add(elif);
                    evalLabel("_if_else_label_" + if_label_counter + ": ;", parent);
                    ++if_label_counter;
                } while (checkElseIf(toks));
                //toks.Next();
                if (toks.Next() == "else")
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

                evalGoto("goto _if_out_label_" + if_counter + ";", (parent.Inner[if_ptr] as If).Body);
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
            if (toks.Next() != "else")
            {
                toks.PushBack();
                return false;
            }
            if (toks.Next() == "if")
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

            cond = "if(!(" + cond + ")) { goto " + for_out + " ; }";
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

            evalGoto("goto " + for_in + ";", forBlock);

            evalLabel(for_out + ": ;", forBlock);

            @for.forBlock = forBlock;
            parent.Inner.Add(@for.forBlock);
            currentFun.cycleStack.Pop();
            //parent.Inner.Add (forBlock);
        }

        private void evalDeclaration(DataTypes t, string sname = "")
        {
            string ident = tokens.Next();
            if (tokens.Next() == "(")
            {
                evalFunction(ident, t, sname); // int main(string args)
            }
            else
            {
                //tokens.PushBack(); 
                evalGlobalVar(ident, t, sname); // public double PI = 3.14;
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
            tokens.Next();
            if (tokens.Type != TokenType.String) throw new CompilerException(ExceptionType.Import, "Import fault: " + tokens.ToString(), tokens);
            dirs.Add(new Import(tokens.ToString()));
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
        private void evalAtributte()
        {
            throw new NotImplementedException();
        }

        private void evalFunction(string ident, DataTypes t, string sname)
        {
            Function f;
            if (t != DataTypes.User)
                f = new Function(ident, t);
            else
                f = new Function(ident, t, new StructType(""));

            f.isNative = isNative;
            f.isPublic = isPublic;

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
                    v = new VarType(VarType.ARG_PREFIX + getIdentifierNext(tokens), DataTypes.User, new StructType(type));
                if (tokens.Next() != "," && tokens.ToString() != ")")
                    throw new CompilerException(ExceptionType.ComaExpected, "',' (coma) expected", tokens);
                f.argTypes.Add(v);
                if (tokens.ToString() == ")")
                    break;
            }

            if (tokens.Next() != "{")
                throw new CompilerException(ExceptionType.Brace, "'{' expected", tokens);
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
            switch (str)
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
            }
        }

        private void evalGlobalVar(string ident, DataTypes t, string sname)
        {
            string expr = ident + " = ";

            ident = VarType.FIELD_PREFIX + ident;
            VarType v;
            if (t != DataTypes.User)
                v = new VarType(ident, t);
            else
                v = new VarType(ident, t, new StructType(""));

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

            var val = tokens.Next();
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
            checkEosNext(tokens);
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
        }
    }
}
