//
//  LangPrimitives.cs
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

using System.Collections.Generic;
using System.Linq;
using CompilerClasses;
using Language;
using System;

namespace Language
{
    enum TokType { String, Int, Double, Function, Variable, TempVar, Boolean, Operator, OperatorEq, OperatorMono, OpenBrc, Keyword };

    struct Token {
        public string StringRep;
        public TokType Type;
        public DataTypes DType;
        
        public Token (string rep, TokType type) {
            StringRep = rep;
            Type = type;
            DType = DataTypes.Void;
        }
        public Token (string rep, TokType type, DataTypes dt) {
            StringRep = rep;
            Type = type;
            DType = dt;
        }
	}

    class BasicPrimitive
    {
        public DataType Type;
        public string Expr;
        public List<Token> Polish;
        public int mainOpIdx;

		public BasicPrimitive() {
			Type = new DataType(DataTypes.Void);
			Expr = "";
			Polish = new List<Token>();
		}

        public BasicPrimitive(DataType t, string expr, List<Token> lt)
        {
            Type = t;
            Expr = expr;
            Polish = lt;
        }
    }

    class CodeBlock
    {
        public CodeBlock Parent = null;
        public List<object> Inner;
        public List<VarDecl> Locals;

        public CodeBlock()
        {
            Inner = new List<object>();
			Locals= new List<VarDecl>();
        }

        public CodeBlock(CodeBlock parent) : this()
        {
            Parent = parent;
        }
    }

    class While
    {
        public BasicPrimitive Cond;
        public object Body;

        public While(BasicPrimitive cond, object body)
        {
            Cond = cond;
            Body = body;
        }
    }

    class For
    {
        public List<VarDecl> Decls;
        public List<object> Init;
        public BasicPrimitive Cond;
        public List<object> Counter;
        public object Body;

		public CodeBlock forBlock;

        public For()
        {
			Decls = new List<VarDecl>();
            Init = new List<object>();
            Counter = new List<object>();
        }
    }

    class If
    {
        public CodeBlock Cond;
        public CodeBlock Body;
        public string Label;

        public List<If> ElseIfs;

        public object Else = null;

        public If(CodeBlock cond, CodeBlock body)
        {
            Cond = cond;
            Body = body;
            ElseIfs = new List<If>();
        }
    }

    class Switch
    {
        public BasicPrimitive Cond;
        //public object Default = null;

        public CodeBlock Body;

        public object Else = null;

        public Switch(BasicPrimitive cond)
        {
            Cond = cond;
            //Cases = new List<Case>();
        }
    }

    class Case
    {
        public BasicPrimitive Constant;
    }

    class Return
    {
        public BasicPrimitive Statement;
    }

    class Goto
    {
        public string Label;
    }

	class Label {
		public string StrLabel;
	}

    class VarDecl
    {
        public VarType Var;
		public int LocalIndex;
    }

    class Import {
        public string Module;

        public Import(string mod) {
            Module = mod;
        }
    }
}

namespace CompilerClasses
{
	enum DataTypes : uint
	{
		Null = 0, Void, Bool, String, User, Byte, Short, Int, Uint, Long, Ulong, Double
	}

	class DataType
	{
		public DataTypes type;
		public StructType user;

		public DataType(DataTypes t, StructType s = null)
		{
			type = t;
			user = s;
		}
	}

	//enum PolishTypes { Digit, Operator, Function, Variable, Equality, OpenBrc, Boolean, String }

	class VarType : DataType
	{
		public string name;
		public bool isNative;
		public bool isPublic;
		public dynamic val;
		
		public const string LOCAL_PREFIX = "_@local_";
		public const string FIELD_PREFIX = "_@field_";
		public const string ARG_PREFIX = "_@arg_";
        public const string NULL_PREFIX = "__@NULL@__";

        public VarType(string name, DataTypes t, StructType s = null)
            : base(t, s)
        {
            this.name = name;
            val = null;
        }

        public VarType(DataTypes t)
            : base(t, null)
        {
            this.name = "";
            val = null;
        }

		internal bool isDigitType()
		{
			if ((int)type < (int)DataTypes.Byte) return false;
			return true;
		}

		public static int GetVarID (string stringRep)
		{
			string sid = "";
			sid = stringRep.Replace (GetVarPrefix (stringRep), "");
			return int.Parse(sid);
		}

		public static int GetVarID (VarType vt)
		{
			return GetVarID (vt.name);
		}

		public static string GetVarPrefix (string stringRep)
		{
			if(stringRep.StartsWith(LOCAL_PREFIX))
				return LOCAL_PREFIX;
			else if(stringRep.StartsWith(FIELD_PREFIX))
				return FIELD_PREFIX;
			else if(stringRep.StartsWith(ARG_PREFIX))
				return ARG_PREFIX;
			return NULL_PREFIX;
		}

		public static string GetVarPrefix (VarType vt)
		{
			return GetVarPrefix (vt.name);
		}

        public override string ToString()
        {
            return name;
        }
	}

	class StructType
	{
		public string name;
		public List<VarType> members;

		public StructType(string name)
		{
			this.name = name;
			members = new List<VarType>();
		}
	}

	class Function : VarType
	{
		public List<VarType> argTypes;
        public Dictionary<int, VarType> locals;
        public string body;
        public string module = "";
		public CodeBlock mainBlock;
		public int tempVarCounter = 0;
        public Stack<Tuple<string, string>> cycleStack;

        public Function(string sign, DataTypes t, StructType s = null)
            : base(sign, t, s)
        {
            argTypes = new List<VarType>();
            locals = new Dictionary<int, VarType> ();
            body = "";
            cycleStack = new Stack<Tuple<string, string>>();
        }

        public Function(string module, string sign, DataTypes ret, List<VarType> args)
            : base(sign, ret, null)
        {
            this.module = module;
            argTypes = args;
            locals = new Dictionary<int, VarType> ();
            body = "";
            cycleStack = new Stack<Tuple<string, string>>();
        }

        public string ArgsFromList() {
            if (argTypes.Count == 0)
                return "";

            string res = "";
            int pos = 0;
            foreach (var a in argTypes)
            {
                res += a.type.ToString().ToLower();
                pos = res.Length;
                res += ", ";
            }

            return res.Remove(pos);
        }

        public static string ArgsFromList(List<VarType> args) {
            if (args.Count == 0)
                return "";

            string res = "";
            int pos = 0;
            foreach (var a in args)
            {
                pos = res.Length;
                res += a.type.ToString().ToLower() + ", ";
            }

            return res.Remove(pos);
        }

        public override string ToString()
        {
            return base.name;
        }
	}

	class FunctionComparer : IEqualityComparer<Function>
	{
		public bool Equals(Function x, Function y)
		{
			if (x.name != y.name) return false;
			if (x.argTypes != y.argTypes) return false;
			return true;
		}

		public int GetHashCode(Function obj)
		{
			return 31 ^ obj.name.Length;
		}
	}

	class OpPriority
	{
		List<string[]> priorityList;

		public OpPriority()
		{
			priorityList = new List<string[]>();
		}

		public void Add(params string[] ops)
		{
			priorityList.Add(ops);
		}

		public int GetPriority(string op)
		{
			for (int i = 0; i < priorityList.Count; ++i)
			{
				var ops = priorityList[i];
				if (ops != null && ops.Contains(op)) return i;
			}
			return -1;
		}

		public int Compare(string op1, string op2)
		{
			var prior1 = GetPriority(op1);
			var prior2 = GetPriority(op2);
			if (prior1 == prior2) return 0;
			else if (prior1 > prior2) return 1;
			else return -1;
		}

		public bool HasBiggerPriority(string op1, string op2)
		{
			var prior1 = GetPriority(op1);
			var prior2 = GetPriority(op2);
			if (prior1 > prior2) return true;
			else return false;
		}

		public bool HasLowerPriority(string op1, string op2)
		{
			var prior1 = GetPriority(op1);
			var prior2 = GetPriority(op2);
			if (prior1 < prior2) return true;
			else return false;
		}

		public bool HasSamePriority(string op1, string op2)
		{
			var prior1 = GetPriority(op1);
			var prior2 = GetPriority(op2);
			if (prior1 == prior2) return true;
			else return false;
		}
	}
}