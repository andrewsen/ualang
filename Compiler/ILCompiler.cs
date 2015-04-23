//
//  ILCompiler.cs
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
using CompilerClasses;
using Language;
using System.Collections.Generic;
using System.IO;

//using System.Reflection.Emit;
using System.Linq;

namespace Translator
{
    enum ILOpCodes : byte
    {
        INV,

        TOP,
        NOP,
        BAND,
        BOR,
        ADD,
        ADDF,
        SUB,
        SUBF,
        MUL,
        MULF,
        DIV,
        DIVF,
        REM,
        REMF,
        CONV_UI8,
        CONV_I16,
        CONV_I32,
        CONV_UI32,
        CONV_I64,
        CONV_UI64,
        CONV_F,
        JMP,
        JZ,
        JT,
        JNZ,
        JF,
        JNULL,
        JNNULL,
        CALL,
        NEWLOC,
        FREELOC,
        LDLOC,
        LDLOC_0,
        LDLOC_1,
        LDLOC_2,
        STLOC,
        STLOC_0,
        STLOC_1,
        STLOC_2,
        LDARG,
        LDARG_0,
        LDARG_1,
        LDARG_2,
        STARG,
        STARG_0,
        STARG_1,
        STARG_2,
        LDFLD,
        LDFLD_0,
        LDFLD_1,
        LDFLD_2,
        STFLD,
        STFLD_0,
        STFLD_1,
        STFLD_2,
        LD_0,
        LD_1,
        LD_2,
        LD_STR,
        LD_UI8,
        LD_I16,
        LD_I32,
        LD_UI32,
        LD_I64,
        LD_UI64,
        LD_F,
        LD_TRUE,
        LD_FALSE,
        LD_NULL,
        AND,
        OR,
        EQ,
        NEQ,
        NOT,
        XOR,
        NEG,
        POS,
        INC,
        DEC,
        SHL,
        SHR,
        POP,
        GT,
        GTE,
        LT,
        LTE,
        SIZEOF,
        TYPEOF,
        RET,
    }

    class ILCompiler
    {
        StreamWriter sw;
        List<Function> funcs;
        CodeBlock global;
        Function currentFun;

        public ILCompiler(List<object> directives, List<Function> funcs, CodeBlock globals, string outF)
        {
            string module = Path.GetFileNameWithoutExtension(outF);

            if (module[0] <= '9' && module[0] >= '0')
            {
                module = "_" + module;
            }

            this.funcs = funcs;
            global = globals;
            //OpCodes.
            sw = new StreamWriter(outF);

            foreach(var obj in directives) {
                if (obj is Import)
                {
                    sw.WriteLine(".import " + (obj as Import).Module);
                    sw.WriteLine("");
                }
            }

            sw.WriteLine(".module " + module);
            sw.WriteLine("");

            sw.WriteLine(".define(\"locale\", \"" + CommandArgs.lang.ToString() + "\")");
            sw.WriteLine(".define(\"source\", \"" + CommandArgs.source + "\")");
            
            if (CommandArgs.noInternal)
            {
                sw.WriteLine(".define(\"internal-api\", \"disabled\")");
            }
            sw.WriteLine("");
            if (CommandArgs.lib)
            {
                sw.WriteLine(".library");
                sw.WriteLine("");
            }
            //sw.WriteLine("");
        }

        public void Compile()
        {
            sw.WriteLine(".globals");
            for (var i = 0; i < global.Locals.Count; ++i)
            {
                var g = global.Locals[i].Var;
                sw.Write(i + " : " + getTypeString(g.type));

                sw.Write(" -> ");
                sw.Write(g.isPublic ? "public " : "private ");
                sw.Write(g.name.Remove(0, VarType.FIELD_PREFIX.Length));
            
                sw.WriteLine("");
                //if (g.val != null)
                //  sw.WriteLine (" = " + g.val.ToString ());
                //else
                //  sw.WriteLine ("");
            }
            sw.WriteLine(".end-globals");

            foreach (var f in funcs)
            {
                sw.WriteLine("");
                currentFun = f;
                sw.Write((f.isPublic ? "public" : "private") + " " + getTypeString(f.type) + " " + f.name + "(");
                for (var i = 0; i < f.argTypes.Count; ++i)
                {
                    var v = f.argTypes[i];
                    sw.Write(getTypeString(v.type));
                    if (i != f.argTypes.Count - 1)
                        sw.Write(", ");
                }
                sw.WriteLine(")");
                if (f.locals.Count != 0)
                {
                    sw.WriteLine("\t.locals");
                    for (int lid = 0; lid < f.locals.Count; ++lid)
                    {
                        sw.WriteLine("\t" + lid + " : " + getTypeString(f.locals[lid].type));
                    }
                    sw.WriteLine("\t.end-locals");
                }
                sw.WriteLine("{");

                if (f.mainBlock.Inner.Count != 0 && !(f.mainBlock.Inner.Last() is Return))
                {
                    var ret = new Return();
                    ret.noData = true;
                    f.mainBlock.Inner.Add(ret);
                }
                writeBlock(f.mainBlock, sw);
                sw.WriteLine("}");
            }

            sw.Close();
        }
   
        void writeBlock(CodeBlock mainBlock, StreamWriter sw)
        {
            /*foreach (var loc in mainBlock.Locals)
            {
                writeOpCode(ILOpCodes.NEWLOC, getTypeString(loc.Var.type));
            }*/

            for (int tokCounter = 0; tokCounter < mainBlock.Inner.Count; ++tokCounter)
            {
                var obj = mainBlock.Inner[tokCounter];

                if (obj is CodeBlock)
                {
                    writeBlock((CodeBlock)obj, sw);
                }
                else if (obj is If)
                {
                    If _if = obj as If;
                    bool not = false;
                    Goto g = null;
                    var counter = _if.Cond.Inner.Count - 1;

                    g = _if.Body.Inner[0] as Goto;
                    if (g != null)
                    {
                        _if.Label = g.Label;
                    }
                  
                    BasicPrimitive lastOp;
                    if(counter != 0)
                        lastOp = _if.Cond.Inner[counter - 1] as BasicPrimitive;
                    else 
                        lastOp = _if.Cond.Inner[counter] as BasicPrimitive;
                    if (lastOp.Polish[lastOp.mainOpIdx].StringRep == "#!")
                    {
                        not = true;
                        --counter;
                    }
                    else if (lastOp.Polish[lastOp.mainOpIdx].StringRep == "!=")
                    {
                        lastOp.Polish[lastOp.mainOpIdx].StringRep = "==";
                        not = true;
                    }
                    for (int i = 0; i < counter; ++i)
                        writeExpr(_if.Cond.Inner[i] as BasicPrimitive);
                    if (not)
                    {
                        writeVal(lastOp.Polish[3]);
                        writeOpCode(ILOpCodes.JZ, _if.Label);
                    }
                    else
                    {
                        writeVal(lastOp.Polish[0]);
                        writeOpCode(ILOpCodes.JNZ, _if.Label);
                    }
                    //sw.WriteLine("\t" + "if " + (_if.Cond.Inner.Last() as BasicPrimitive).Expr);
                    if(g == null)
                        writeBlock(_if.Body as CodeBlock, sw);
                }
                else if (obj is Goto)
                {
                    Goto g = obj as Goto;
                    writeOpCode(ILOpCodes.JMP, g.Label);
                    for (;tokCounter < mainBlock.Inner.Count; ++tokCounter) //TEST
                    {
                        if (mainBlock.Inner[tokCounter] is Language.Label)
                        {
                            --tokCounter;
                            break;
                        }
                    }
                }
                else if (obj is Language.Label)
                {
                    Label l = obj as Label;
                    sw.WriteLine(l.StrLabel + ":");
                }
                else if (obj is While)
                {
                    While w = obj as While;
                    writeBlock(w.Body as CodeBlock, sw);
                }
                else if (obj is Return)
                {
                    Return r = obj as Return;
                    if (currentFun.type != DataTypes.Void && !r.noData)
                    {
                        var val = r.Statement.Polish[0];
                        writeVal(val);
                    }
                    writeOpCode(ILOpCodes.RET);
                    return;
                }
                else if (obj is For)
                {
                    For f = obj as For;
                    writeBlock(f.forBlock, sw);
                }
                else if (obj is BasicPrimitive)
                {
                    var bp = obj as BasicPrimitive;
                    if (bp.Polish.Count == 1 && bp.Polish[0].Type != TokType.Function)
                        continue;//bp.Expr = "//" + bp.Expr;
                    writeExpr(bp);
                }
            }

            /*foreach (var loc in mainBlock.Locals)
            {
                writeOpCode(ILOpCodes.FREELOC, loc.LocalIndex);
            }*/

        }

        void writeExpr(BasicPrimitive bp)
        {
            var op = bp.Polish[bp.mainOpIdx];
            if (op.StringRep == "=")
            {
                var lval = bp.Polish[0];
                var rval = bp.Polish[2];
                if (lval.Type != TokType.Variable && lval.Type != TokType.TempVar)
                    throw new CompilerException(ExceptionType.lValueExpected, "lvalue expected", null);

                if (rval.Type == TokType.Variable)
                {
                    if (rval.StringRep == lval.StringRep)
                        return;

                    if (rval.StringRep.StartsWith(VarType.FIELD_PREFIX))
                        writeShortcut(VarType.GetVarID(rval.StringRep), ILOpCodes.LDFLD);
                        //writeOpCode(ILOpCodes.LDFLD, VarType.GetVarID(rval.StringRep));
                    else if (rval.StringRep.StartsWith(VarType.ARG_PREFIX))
                        writeShortcut(VarType.GetVarID(rval.StringRep), ILOpCodes.LDARG);
                        //writeOpCode(ILOpCodes.LDARG, VarType.GetVarID(rval.StringRep));
                    else
                        writeShortcut(VarType.GetVarID(rval.StringRep), ILOpCodes.LDLOC);
                        //writeOpCode(ILOpCodes.LDLOC, VarType.GetVarID(rval.StringRep));
                }
                else if (rval.Type == TokType.Double) 
                    writeOpCode(ILOpCodes.LD_F, rval.StringRep);
                else if (rval.Type == TokType.Int)
                    writeShortcut(rval.StringRep, ILOpCodes.LD_I32, ILOpCodes.LD_0, ILOpCodes.LD_1, ILOpCodes.LD_2);
                else if (rval.Type == TokType.String) 
                    writeOpCode(ILOpCodes.LD_STR, rval.StringRep);
                else if (rval.Type == TokType.Boolean)
                {
                    if(rval.StringRep == "true")
                        writeOpCode(ILOpCodes.LD_TRUE);
                    else
                        writeOpCode(ILOpCodes.LD_FALSE);
                }

                if (lval.StringRep.StartsWith(VarType.FIELD_PREFIX))
                    writeShortcut(VarType.GetVarID(lval.StringRep), ILOpCodes.STFLD);
                    //writeOpCode(ILOpCodes.STFLD, VarType.GetVarID(lval.StringRep));
                else if (lval.StringRep.StartsWith(VarType.ARG_PREFIX))
                    writeShortcut(VarType.GetVarID(lval.StringRep), ILOpCodes.STARG);
                    //writeOpCode(ILOpCodes.STARG, VarType.GetVarID(lval.StringRep));
                else if (lval.StringRep.StartsWith(VarType.LOCAL_PREFIX))
                    writeShortcut(VarType.GetVarID(lval.StringRep), ILOpCodes.STLOC);
                    //writeOpCode(ILOpCodes.STLOC, VarType.GetVarID(lval.StringRep));
            }
            else if (op.StringRep == "_@call@_" || op.StringRep == "_@call_noreturn@_")
            {
                var fun = bp.Polish[bp.mainOpIdx + 1];
                bool temps = true;
                //printf(...) loc0, t1, t2, arg3, t4
                // ; t1 t2 t4
                //ldloc 0 ;  t1 t2 t4 loc0
                //top 3 ; t2 t4 loc0 t1 
                //top 3 ; t4 loc0 t1 t2
                //ldarg 3 ; t4 loc0 t1 t2 arg3
                //top 4 ; loc0, t1, t2, arg3, t4
                int tempVars = -1;
                for (int i = bp.mainOpIdx + 2; i < bp.Polish.Count; ++i)
                {
                    if (bp.Polish[i].Type == TokType.TempVar)
                        ++tempVars;
                }
                for (int i = bp.mainOpIdx + 2; i < bp.Polish.Count; ++i)
                {
                    if (bp.Polish[i].Type != TokType.TempVar && temps)
                        temps = false;
                    if (bp.Polish[i].Type == TokType.TempVar && !temps)
                        writeOpCode(ILOpCodes.TOP, tempVars);
                    else
                    {
                        writeVal(bp.Polish[i]);
                        ++tempVars;
                    }
                }
                writeOpCode(ILOpCodes.CALL, fun.StringRep);
                if(op.StringRep == "_@call_noreturn@_")
                    writeOpCode(ILOpCodes.POP);
            }
            else if (op.StringRep == "_@as@_")
            {
                writeVal(bp.Polish[2]);
                switch (bp.Polish[4].DType)
                {
                    case DataTypes.Byte:
                        writeOpCode(ILOpCodes.CONV_UI8);
                        break;
                    case DataTypes.Short:
                        writeOpCode(ILOpCodes.CONV_I16);
                        break;
                    case DataTypes.Int:
                        writeOpCode(ILOpCodes.CONV_I32);
                        break;
                    case DataTypes.Uint:
                        writeOpCode(ILOpCodes.CONV_UI32);
                        break;
                    case DataTypes.Long:
                        writeOpCode(ILOpCodes.CONV_I64);
                        break;
                    case DataTypes.Ulong:
                        writeOpCode(ILOpCodes.CONV_UI64);
                        break;
                    case DataTypes.Double:
                        writeOpCode(ILOpCodes.CONV_F);
                        break;
                    default:
                        break;
                }
            }
            else if (bp.Polish.Count == 5)
            {
                var val1 = bp.Polish[2];
                var val2 = bp.Polish[4];
                if (bp.Polish[0].Type != TokType.TempVar)
                    throw new CompilerException(ExceptionType.lValueExpected, "tempVar expected", null);
                if (val1.Type != TokType.TempVar && val2.Type == TokType.TempVar)
                {
                    //throw new CompilerException(ExceptionType.FlowError, 
                    //                            "Control flow error in operator: " + val1.StringRep + " " + op.StringRep + " " + val2.StringRep, null);
                    writeVal(val1);
                    writeOpCode(ILOpCodes.TOP, 1);
                    writeOpCode(getOpCode(op.StringRep, val1.DType, val2.DType));
                }
                else
                {
                    writeVal(val1);
                    writeVal(val2);
                    writeOpCode(getOpCode(op.StringRep, val1.DType, val2.DType));
                }
            }
            else
            {
                writeVal(bp.Polish[3]);
                writeOpCode(getOpCode(op.StringRep, bp.Polish[3].DType));
            }

        }

        void writeOpCode(ILOpCodes op, params object[] args)
        {
            var line = op.ToString().ToLower() + " ";
            foreach (var a in args)
            {
                line += a.ToString() + " ";
            }
            line.Trim();
            sw.WriteLine("\t" + line);
        }

        void writeOpCode(ILOpCodes op)
        {
            var line = op.ToString().ToLower();
            sw.WriteLine("\t" + line);
        }

        void writeVal(Token val)
        {                        
            if (val.Type == TokType.Variable)
            {
                if (val.StringRep.StartsWith(VarType.FIELD_PREFIX))
                    writeShortcut(VarType.GetVarID(val.StringRep), ILOpCodes.LDFLD);
                else if (val.StringRep.StartsWith(VarType.ARG_PREFIX))
                    writeShortcut(VarType.GetVarID(val.StringRep), ILOpCodes.LDARG);
                else
                    writeShortcut(VarType.GetVarID(val.StringRep), ILOpCodes.LDLOC);
            }
            else if (val.Type == TokType.Double) 
                writeOpCode(ILOpCodes.LD_F, val.StringRep);
            else if (val.Type == TokType.Int) 
                writeShortcut(val.StringRep, ILOpCodes.LD_I32, ILOpCodes.LD_0, ILOpCodes.LD_1, ILOpCodes.LD_2);
            else if (val.Type == TokType.String) 
                writeOpCode(ILOpCodes.LD_STR, val.StringRep);
            else if (val.Type == TokType.Boolean) 
            {
                if(val.StringRep == "true")
                    writeOpCode(ILOpCodes.LD_TRUE);
                else
                    writeOpCode(ILOpCodes.LD_FALSE);
            }
        }

        ILOpCodes getOpCode(string op, DataTypes t1, DataTypes t2 = DataTypes.Void)
        {
            switch (op)
            {
            /*            priorities.Add("=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=");
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
            priorities.Add("++", "--", ".");*/
                case "==":
                    return ILOpCodes.EQ;
                case "!=":
                    return ILOpCodes.NEQ;
                case "||":
                    return ILOpCodes.OR;
                case "&&":
                    return ILOpCodes.AND;
                case "|":
                    return ILOpCodes.OR;
                case "&":
                    return ILOpCodes.AND;
                case "^":
                    return ILOpCodes.XOR;
                case ">":
                    return ILOpCodes.GT;
                case ">=":
                    return ILOpCodes.GTE;
                case "<=":
                    return ILOpCodes.LTE;
                case "<":
                    return ILOpCodes.LT;
                case "#~":
                    return ILOpCodes.INV;
                case "#!":
                    return ILOpCodes.NOT;
                case "#+":
                    return ILOpCodes.POS;
                case "#-":
                    return ILOpCodes.NEG;
                case ">>":
                    return ILOpCodes.SHR;
                case "<<":
                    return ILOpCodes.SHL;
                case "++":
                    return ILOpCodes.INC;
                case "--":
                    return ILOpCodes.DEC;
                case "%":
                    if (t1 != DataTypes.Double)
                        return ILOpCodes.REM;
                    else
                        return ILOpCodes.REMF;
                case "+":
                    if (t1 != DataTypes.Double)
                        return ILOpCodes.ADD;
                    else
                        return ILOpCodes.ADDF;
                case "-":
                    if (t1 != DataTypes.Double)
                        return ILOpCodes.SUB;
                    else
                        return ILOpCodes.SUBF;
                case "*":
                    if (t1 != DataTypes.Double)
                        return ILOpCodes.MUL;
                    else
                        return ILOpCodes.MULF;
                case "/":
                    if (t1 != DataTypes.Double)
                        return ILOpCodes.DIV;
                    else
                        return ILOpCodes.DIVF;
                default:
                    return ILOpCodes.NOP;
            }
        }

        public static string getTypeString(DataTypes type)
        {
            switch (type)
            {
                case DataTypes.Byte:
                    return "ui8";
                case DataTypes.Short:
                    return "i16";
                case DataTypes.Int:
                    return "i32";
                case DataTypes.Uint:
                    return "ui32";
                case DataTypes.Long:
                    return "i64";
                case DataTypes.Ulong:
                    return "ui64";
                default:
                    return type.ToString().ToLower();
            }
        }
        
        void writeShortcut(string rep, ILOpCodes op) {
            writeShortcut(int.Parse(rep), op);
        }

        void writeShortcut(int rep, ILOpCodes op) {
            if (rep >= 0 && rep <= 2)
                sw.WriteLine("\t"+op.ToString().ToLower() + "_" + rep);
            else
                writeOpCode(op, rep);
        }

        void writeShortcut(string rep, ILOpCodes op, ILOpCodes short0, ILOpCodes short1, ILOpCodes short2) {
            writeShortcut(int.Parse(rep), op, short0, short1, short2);
        }

        void writeShortcut(int rep, ILOpCodes op, ILOpCodes short0, ILOpCodes short1, ILOpCodes short2) {
            if (rep == 0)
                writeOpCode(short0);
            else if (rep == 1)
                writeOpCode(short1);
            else if (rep == 2)                    
                writeOpCode(short2);
            else
                writeOpCode(op, rep);
        }
    }
}

