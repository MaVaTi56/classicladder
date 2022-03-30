/* Classic Ladder Project */
/* Copyright (C) 2001-2021 Marc Le Douarain */
/* http://www.sourceforge.net/projects/classicladder */
/* http://sites.google.com/site/classicladder */
/* October 2001 */
/* ------------------------------- */
/* Arithmetic expression evaluator */
/* ------------------------------- */
/* This library is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU Lesser General Public */
/* License as published by the Free Software Foundation; either */
/* version 2.1 of the License, or (at your option) any later version. */

/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/* Lesser General Public License for more details. */

/* You should have received a copy of the GNU Lesser General Public */
/* License along with this library; if not, write to the Free Software */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */

#ifdef MODULE
#include <linux/string.h>
#ifdef RTAI
#include <linux/kernel.h>
#include <linux/module.h>
#include "rtai.h"
#endif
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "classicladder.h"
#include "global.h"
#include "vars_access.h"
#ifdef GTK_INTERFACE
#include "vars_names.h"
#endif
#include "arithm_eval.h"


char * Expr;
char * ErrorDesc;
char * VerifyErrorDesc;
int UnderVerify;

/* for RTLinux module */
#if defined( MODULE )
int atoi(const char *p)
{
	int n=0;
	while(*p>='0' && *p<='9')
		n = n*10 + *p++-'0';
	return n;
}
#endif
int pow_int(int a,int b)
{
	int res;
	if ( b==0 )
	{
		res = 1;
	}
	else
	{
		int x;
		res = a;
		for (x=1;x<b;x++)
		{
			res = res*a;
//printf("x%d, val=%d\n", x, res );
		}
	}
	return res;
}


void SyntaxError(void)
{
	if (UnderVerify)
		VerifyErrorDesc = ErrorDesc;
	else
		rt_debug_printf("Syntax error : '%s' , at %s !!!!!\n",ErrorDesc,Expr);
}

arithmtype Constant(void)
{
	arithmtype Res = 0;
	char cIsNeg = FALSE;
	/* negative constant ? */
	if ( *Expr=='-' )
	{
		cIsNeg = TRUE;
		Expr++;
	}
	if (*Expr=='$' )
	{
		Expr++;
		/* hexa number */
		while( (*Expr>='0' && *Expr<='9') || (*Expr>='A' && *Expr<='F')  || (*Expr>='a' && *Expr<='f') )
		{
			char Carac = *Expr;
			if ( Carac>='A' && Carac<='F' )
				Carac = Carac-'A'+10;
			else
			if ( Carac>='a' && Carac<='f' )
				Carac = Carac-'a'+10;
			else
				Carac = Carac-'0';
			Res = 16*Res + Carac;
			Expr++;
		}
	}
	else if (*Expr=='\'')
	{
		Expr++;
		/* character constant */
		while( *Expr!='\'' && *Expr!='\0' )
			Res = 256*Res + (*Expr++);
		if ( *Expr!='\'' )
		{
			ErrorDesc = "Missing end ' in character constant";
			SyntaxError();
			return 0;
		}
	}
	else
	{
		/* decimal number */
		while(*Expr>='0' && *Expr<='9')
		{
			Res = 10*Res + (*Expr-'0');
			Expr++;
		}
	}
	if ( cIsNeg )
		Res = Res * -1;
	return Res;
}

/* return TRUE if okay: pointer of pointer on ONE var : "xxx/yyy@" or "xxx/yyy[" */
/* pointer of pointer is advanced to the first character after last y  : @ or [ */
int IdentifyVarContent( char **PtrStartExpr, int * ResType,int * ResOffset )
{
	char * StartExpr = *PtrStartExpr;
	char * SearchSep = StartExpr;
//printf("IndentifyVar=%s\n", StartExpr);

	do
	{
		SearchSep++;
	}
	while( (*SearchSep!='/') && (*SearchSep!='\0') );

	if (*SearchSep=='/')
	{
		int VarType = atoi(StartExpr);
		SearchSep++;
		StartExpr = SearchSep;
		do
		{
			StartExpr++;
		}
		while( (*StartExpr!='@') && (*StartExpr!='[') && (*StartExpr!='\0') );
		if (*StartExpr=='@' || *StartExpr=='[')
		{
			int VarOffset = atoi(SearchSep);
			*ResType = VarType;
			*ResOffset = VarOffset;

			*PtrStartExpr = StartExpr;
//printf("IdentifyVarResult=%d/%d\n", VarType, VarOffset);
			return TRUE;
		}
		else
		{
			ErrorDesc = "Bad var coding (err=2), should be @xx/yy@";
			SyntaxError();
			return FALSE;
		}
	}
	else
	{
		ErrorDesc = "Bad var coding (err=1), should have a / for xx/yy form";
	}
	SyntaxError();
	return FALSE;
}

/* return TRUE if okay: pointer on a simple var or indexed one : "@xxx/yyy@" or @xxx/yyy[xxx/yyy]@" */
int IdentifyVarIndexedOrNot(char *StartExpr,int * ResType,int * ResOffset, int * ResIndexType,int * ResIndexOffset)
{
	int VarType,VarOffset;
	char * ScanExpr = StartExpr;
//printf("START identify var at:%s...\n", ScanExpr);
	if (*ScanExpr!='@')
	{
		ErrorDesc = "Bad var coding (err=0), should start with @ for @xx/yy@ form";
		SyntaxError();
		return FALSE;
	}
	ScanExpr++;
	// no index per default!
	*ResIndexType = -1;
	*ResIndexOffset = -1;

	if ( IdentifyVarContent( &ScanExpr, &VarType, &VarOffset ) )
	{
		*ResType = VarType;
		*ResOffset = VarOffset;
		// there is an index present ?
		if ( *ScanExpr=='[')
		{
			ScanExpr++;
			if ( IdentifyVarContent( &ScanExpr, &VarType, &VarOffset ) )
			{
				*ResIndexType = VarType;
				*ResIndexOffset = VarOffset;
//printf("identified indexed var for %s => %d/%d[%d/%d]\n", StartExpr, *ResType, *ResOffset, *ResIndexType, *ResIndexOffset );
				return TRUE;
			}
		}
		else
		{
//printf("identified simple var for %s => %d/%d\n", StartExpr, *ResType, *ResOffset );
			return TRUE;
		}
	}
	ErrorDesc = "Bad var coding (unknown variable)";
	SyntaxError();
	return FALSE;
}

/* Give final variable (taking into account the value of an index if present) */
int IdentifyFinalVar( char *StartExpr, int * ResType,int * ResOffset )
{
	int IndexVarType,IndexVarOffset;
	int SyntaxOk = IdentifyVarIndexedOrNot( StartExpr, ResType, ResOffset, &IndexVarType, &IndexVarOffset );
	if ( SyntaxOk )
	{
		if ( IndexVarType!=-1 && IndexVarOffset!=-1 )
		{
			// add index value from content of the index variable
			int IndexValue = ReadVar( IndexVarType, IndexVarOffset );
			*ResOffset = *ResOffset + IndexValue;
		}
//printf("Final var for %s => %d/%d\n", StartExpr, *ResType, *ResOffset );
	}
	return SyntaxOk;
}

arithmtype Variable(void)
{
	int VarType,VarOffset;
	if (IdentifyFinalVar(Expr, &VarType,&VarOffset))
	{
//printf("Variable:%d/%d\n", VarType, VarOffset);
		/* flush var found */
		Expr++;
		do
		{
			Expr++;
		}
		while( (*Expr!='@') && (*Expr!='\0') );
		Expr++;
		/* return var value */
		return (arithmtype)ReadVar(VarType,VarOffset);
	}
	else
	{
		return 0;
	}
}

arithmtype VariableOrConstant(void)
{
	if (*Expr=='@' )
	{
		return Variable( );
	}
	else if( (*Expr>='0' && *Expr<='9') || *Expr=='-' || *Expr=='$' || *Expr=='\'')
	{
		return Constant( );
	}
	else
	{
		ErrorDesc = "Parameter in a function that is neither a variable nor a constant !";
		SyntaxError();
	}
	return 0;
}

arithmtype Function(void)
{
	char tcFonc[ 20 ], *pFonc, *pFoncExprForError;
	int Res = 0;

	pFoncExprForError = Expr;
	/* which function ? */
	pFonc = tcFonc;
	while((unsigned int)(pFonc-tcFonc)<sizeof(tcFonc)-1 && *Expr>='A' && *Expr<='Z')
	{
		*pFonc++ = *Expr;
		Expr++;
	}
	*pFonc = '\0';

	/* functions with one parameter = variable */
	if ( !strcmp(tcFonc, "ABS") )
	{
		Expr++; /* ( */
		Res = VariableOrConstant( );
		if ( Res<0 )
			Res = Res * -1;
		if ( *Expr!=')' )
		{
			ErrorDesc = "Missing end ) after the only variable in ABS() function";
			SyntaxError();
			return 0;
		}
		Expr++; /* ) */
		return Res;
	}

	/* functions with many parameters = many variables separated per ',' */
	if ( !strcmp(tcFonc, "MINI") )
	{
		Res = 0x7FFFFFFF;
		do
		{
			int iValVar;
			Expr++; /* ( -ou- , */
			iValVar = VariableOrConstant( );
			if ( iValVar<Res )
				Res = iValVar;
		}
		while( *Expr!=')' );
		Expr++; /* ) */
		return Res;
	}
	if ( !strcmp(tcFonc, "MAXI") )
	{
		Res = 0x80000000;
		do
		{
			int iValVar;
			Expr++; /* ( -or- , */
			iValVar = VariableOrConstant( );
			if ( iValVar>Res )
				Res = iValVar;
		}
		while( *Expr!=')' );
		Expr++; /* ) */
		return Res;
	}
	if ( !strcmp(tcFonc, "MOY") /*original french term!*/ || !strcmp(tcFonc, "AVG") /*added latter!!!*/ )
	{
		int NbrVars = 0;
		do
		{
			int ValVar;
			Expr++; /* ( -or- , */
			ValVar = VariableOrConstant( );
			NbrVars++;
			Res = Res + ValVar;
		}
		while( *Expr!=')' );
		Expr++; /* ) */
		Res = Res/NbrVars;
		return Res;
	}
	if ( !strcmp(tcFonc, "POW") )
	{
		int NbrParams = 0;
		int ValFirstParam = 0;
		do
		{
			Expr++; /* ( -or- , */
			if ( NbrParams==0 )
				ValFirstParam = VariableOrConstant( );
			else
				Res = VariableOrConstant( );
			NbrParams++;
		}
		while( *Expr!=')' && NbrParams<2 );
		if ( *Expr!=')' )
		{
			ErrorDesc = "Missing end ) in POW(a,b)";
			SyntaxError();
			return 0;
		}
		Expr++; /* ) */
//printf("POW: a=%d, b=%d\n", ValFirstParam, Res );
		Res = pow_int(ValFirstParam,Res);
//printf("POW: res=%d\n", Res );
		return Res;
	}
	if ( !strcmp(tcFonc, "SHL") || !strcmp(tcFonc, "ROL") ) /* Shift or Rotate Left */
	{
		int NbrParams = 0;
		int ValFirstParam = 0;
		char BitOut = FALSE;
		do
		{
			Expr++; /* ( -or- , */
			if ( NbrParams==0 )
				ValFirstParam = VariableOrConstant( );
			else
				Res = VariableOrConstant( );
			NbrParams++;
		}
		while( *Expr!=')' && NbrParams<2 );
		if ( *Expr!=')' )
		{
			ErrorDesc = "Missing end ) in SLW(a,i) or ROL(a,i)";
			SyntaxError();
			return 0;
		}
		Expr++; /* ) */
//printf("SHL/ROL: a=%d, i=%d\n", ValFirstParam, Res );
		BitOut = (ValFirstParam&(1<<31))?TRUE:FALSE;
		Res = ValFirstParam<<Res;
		// rotate case, with bit out use.
		if( tcFonc[0]=='R' && BitOut )
			Res = Res | 1;
		WriteVar( VAR_SYSTEM, 8, BitOut );
//printf("SHL/ROL: res=%d, bit_out=%d\n", Res, BitOut );
		return Res;
	}
	if ( !strcmp(tcFonc, "SHR") || !strcmp(tcFonc, "ROR") ) /* Shift or Rotate Right */
	{
		int NbrParams = 0;
		int ValFirstParam = 0;
		char BitOut = FALSE;
		do
		{
			Expr++; /* ( -or- , */
			if ( NbrParams==0 )
				ValFirstParam = VariableOrConstant( );
			else
				Res = VariableOrConstant( );
			NbrParams++;
		}
		while( *Expr!=')' && NbrParams<2 );
		if ( *Expr!=')' )
		{
			ErrorDesc = "Missing end ) in SRW(a,i) or ROR(a,i)";
			SyntaxError();
			return 0;
		}
		Expr++; /* ) */
//printf("SHR/ROR: a=%d, i=%d\n", ValFirstParam, Res );
		BitOut = (ValFirstParam&1)?TRUE:FALSE;
		Res = (ValFirstParam>>Res)&0x7FFFFFFF;
		// rotate case, with bit out use.
		if( tcFonc[0]=='R' && BitOut )
			Res = Res | (1<<31);
		WriteVar( VAR_SYSTEM, 8, BitOut );
//printf("SHR/ROR: res=%d, bit_out=%d\n", Res, BitOut );
		return Res;
	}

	/* functions with parameter = term */
//	int iValeurTerm = Term();
//	if ( !strcmp(tcFonc, "") )
//	{
//	}


	ErrorDesc = "Unknown function";
	//0.9.113, added to give function name not found !
	Expr = pFoncExprForError;
printf( "ERROR IN %s() at %s\n", __FUNCTION__, Expr );
	SyntaxError();

	return 0;
}

arithmtype Term(void)
{
//if (UnderVerify)
//printf("Term_Expr=%s (%c)\n",Expr, *Expr);
	if (*Expr=='(')
	{
		arithmtype Res;
		Expr++;
//		Res = AddSub();
		Res = Or();
		if (*Expr!=')')
		{
			ErrorDesc = "Missing parenthesis )";
			SyntaxError();
		}
		Expr++;
		return Res;
	}
	else if ( (*Expr>='0' && *Expr<='9') || (*Expr=='$') || (*Expr=='-') || (*Expr=='\'') )
		return Constant();
	else if (*Expr>='A' && *Expr<='Z')
		return Function();
	else if (*Expr=='@')
	{
		return Variable();
	}
	else if (*Expr=='!')
	{
		Expr++;
		return Term()?0:1;
	}
	else
	{
if (UnderVerify)
debug_printf("TermERROR!_ExprHere=%s\n",Expr);
		ErrorDesc = "Unknown term";
		SyntaxError();
		return 0;
	}
//	return 0;
}

/*arithmtype Pow(void)
{
	arithmtype Q,Res = Term();
//	while(*Expr=='^')
	while(*Expr==';')
	{
		if ( ErrorDesc )
			break;
		Expr++;
		Q = Pow();
		Res = pow_int(Res,Q);
	}
	return Res;
}*/

arithmtype MulDivMod(void)
{
	int Val=0;
	arithmtype Res = Term(); //Pow();
	while(1)
	{
		if ( ErrorDesc )
			break;
//if (UnderVerify)
//printf("MulDivMod_Expr=%s\n",Expr);
		if (*Expr=='*')
		{
			Expr++;
			Res = Res * Term(); //Pow();
		}
		else
		if (*Expr=='/')
		{
			Expr++;
			Val = Term(); //Pow();
			if ( ErrorDesc==NULL )
			{
				//0.9.113, verify if not null !
				if( Val!=0 )
				{
					Res = Res / Val;
				}
				else
				{
					Res = 0;
					if( !UnderVerify )
						WriteVar( VAR_SYSTEM, 7, TRUE );
				}
			}
		}
		else
		if (*Expr=='%')
		{
			Expr++;
			Val = Term(); //Pow();
			if ( ErrorDesc==NULL )
			{
				//0.9.113, verify if not null !
				if( Val!=0 )
				{
					Res = Res % Val;
				}
				else
				{
					Res = 0;
					if( !UnderVerify )
						WriteVar( VAR_SYSTEM, 7, TRUE );
				}
			}
		}
		else
		{
			break;
		}
	}
	return Res;
}

arithmtype AddSub(void)
{
	arithmtype Res = MulDivMod();
	while(1)
	{
		if ( ErrorDesc )
			break;
//if (UnderVerify)
//printf("AddSub_Expr=%s\n",Expr);
		if (*Expr=='+')
		{
			Expr++;
			Res = Res + MulDivMod();
		}
		else
		if (*Expr=='-')
		{
			Expr++;
			Res = Res - MulDivMod();
		}
		else
		{
			break;
		}
	}
	return Res;
}

arithmtype And(void)
{
	arithmtype Res = AddSub();
	while(1)
	{
		if ( ErrorDesc )
			break;
		if (*Expr=='&')
		{
			Expr++;
			Res = Res & AddSub();
		}
		else
		{
			break;
		}
	}
	return Res;
}
arithmtype Xor(void)
{
	arithmtype Res = And();
	while(1)
	{
		if ( ErrorDesc )
			break;
		if (*Expr=='^')
		{
			Expr++;
			Res = Res ^ And();
		}
		else
		{
			break;
		}
	}
	return Res;
}
arithmtype Or(void)
{
	arithmtype Res = Xor();
	while(1)
	{
		if ( ErrorDesc )
			break;
		if (*Expr=='|')
		{
			Expr++;
			Res = Res | Xor();
		}
		else
		{
			break;
		}
	}
	return Res;
}

arithmtype EvalExpression(char * ExprString)
{
	arithmtype Res;
	Expr = ExprString;
//    Res = AddSub();
	ErrorDesc = NULL;
	Res = Or();

	//0.9.113, to not loose previous error message
	if( ErrorDesc==NULL )
	{
		//verify added in 0.9.4
		if ( *Expr!='\0' )
		{
			if (*Expr==')')
			{
				ErrorDesc = "Excess parenthesis )";
				SyntaxError();
			}
			else
			{
//ToFixInTranslate			ErrorDesc = "Unknown characters when evaluating !?";
				ErrorDesc = "Unknown characters when evaluating !?";
				SyntaxError();
			}
		}
	}

	return Res;
}


//outside function to return error position on it !
char StrCopyEvalCompare[ARITHM_EXPR_SIZE+1]; /* used for putting null char after first expr */
/* Result of the comparison of 2 arithmetics expressions : */
/* Expr1 ... Expr2 where ... can be : < , > , = , <= , >= , <> */
int EvalCompare(char * CompareString)
{
	char * FirstExpr,* SecondExpr = NULL;
	char * SearchSep;
	char * CutFirst;
	int Found = FALSE;
	int BoolRes = 0;

	/* null expression ? */
	if (*CompareString=='\0' || *CompareString=='#')
		return BoolRes;

	strcpy(StrCopyEvalCompare,CompareString);

	/* search for '>' or '<' or '=' or '>=' or '<=' */
	CutFirst = FirstExpr = StrCopyEvalCompare;
	SearchSep = CompareString;
	do
	{
		if ( (*SearchSep=='>') || (*SearchSep=='<') || (*SearchSep=='=') )
		{
			Found = TRUE;
			*CutFirst = '\0';
			CutFirst++;
			SecondExpr = CutFirst;
			/* 2 chars if '>=' or '<=' or '<>' */
			if ( *CutFirst=='=' || *CutFirst=='>')
			{
				CutFirst++;
				SecondExpr = CutFirst;
			}
		}
		else
		{
			SearchSep++;
			CutFirst++;
		}
	}
	while (*SearchSep!='\0' && !Found);
	if (Found)
	{
		arithmtype EvalFirst,EvalSecond;
//printf("EvalCompare FirstString=%s , SecondString=%s\n",FirstExpr,SecondExpr);
		EvalFirst = EvalExpression(FirstExpr);
		EvalSecond = EvalExpression(SecondExpr);
//printf("EvalCompare ResultFirst=%d , ResultSecond=%d\n",EvalFirst,EvalSecond);
		/* verify if compare is true */
		if ( *SearchSep=='>' && EvalFirst>EvalSecond )
			BoolRes = 1;
		if ( *SearchSep=='<' && *(SearchSep+1)!='>' && EvalFirst<EvalSecond )
			BoolRes = 1;
		if ( *SearchSep=='<' && *(SearchSep+1)=='>' && EvalFirst!=EvalSecond )
			BoolRes = 1;
		if ( (*SearchSep=='=' || *(SearchSep+1)=='=') && EvalFirst==EvalSecond )
			BoolRes = 1;
	}
	else
	{
		ErrorDesc = "Missing < or > or = or ... to make compare";
		SyntaxError();
	}
//printf("Eval FinalBoolResult = %d\n",BoolRes);
	return BoolRes;
}

//outside function to return error position on it !
char StrCopyMakeCalc[ARITHM_EXPR_SIZE+1]; /* used for putting null char after first expr */
/* Calc the new value of a variable from an arithmetic expression : */
/* VarDest := ArithmExpr */
void MakeCalc(char * CalcString,int VerifyMode)
{
	int TargetVarType,TargetVarOffset;

	/* null expression ? */
	if (*CalcString=='\0' || *CalcString=='#')
		return;

	strcpy(StrCopyMakeCalc,CalcString);

	Expr = StrCopyMakeCalc;
	if (IdentifyFinalVar(Expr,&TargetVarType,&TargetVarOffset))
	{
		int Found = FALSE;
		/* flush var found */
		Expr++;
		do
		{
			Expr++;
		}
		while( (*Expr!='@') && (*Expr!='\0') );
		Expr++;
		/* verify if there is the '=' or ':=' */
		do
		{
			if (*Expr==':')
				Expr++;
			if (*Expr=='=')
			{
				Found = TRUE;
				Expr++;
			}
			if (*Expr==' ')
				Expr++;
		}
		while( !Found && *Expr!='\0' );
		while( *Expr==' ')
			Expr++;
		if (Found)
		{
			arithmtype EvalExpr;
//printf("Calc - Eval String=%s\n",Expr);
			EvalExpr = EvalExpression(Expr);
//printf("Calc - Result=%d (StringAt=%s)\n",EvalExpr,Expr);
			if (!VerifyMode)
			{
				WriteVar(TargetVarType,TargetVarOffset,(int)EvalExpr);
			}
#ifdef GTK_INTERFACE
			else
			{
				if ( !TestVarIsReadWrite( TargetVarType, TargetVarOffset ) )
				{
					ErrorDesc = "Target variable must be read/write !";
					SyntaxError();
				}
			}
#endif
		}
		else
		{
			ErrorDesc = "Missing := to make operate";
			SyntaxError();
		}
	}
}

/* Used one time after user input to verify syntax only */
/* return NULL if ok, else pointer on error description */
/* 0.9.113, added ErrorAt pointer of pointer... to return where error was encountered */
char * VerifySyntaxForEvalCompare(char * StringToVerify, char ** ErrorAt )
{
	UnderVerify = TRUE;
	VerifyErrorDesc = NULL;
	EvalCompare(StringToVerify);
	(*ErrorAt) = Expr;
	UnderVerify = FALSE;
	return VerifyErrorDesc;
}
/* Used one time after user input to verify syntax only */
/* return NULL if ok, else pointer on error description */
/* 0.9.113, added ErrorAt pointer of pointer... to return where error was encountered */
char * VerifySyntaxForMakeCalc(char * StringToVerify, char ** ErrorAt)
{
	UnderVerify = TRUE;
	VerifyErrorDesc = NULL;
//printf("%s() - StartEval String=%s\n",__FUNCTION__,StringToVerify);
	MakeCalc(StringToVerify,TRUE /* verify mode */);
//printf("%s() - Endval String=%s\n",__FUNCTION__,Expr);
	(*ErrorAt) = Expr;
	UnderVerify = FALSE;
	return VerifyErrorDesc;
}

