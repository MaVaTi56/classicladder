CLASSIC LADDER PROJECT
Copyright (C) 2001-2021 Marc Le Douarain
marc . le - douarain /AT\ laposte \DOT/ net
http://www.sourceforge.net/projects/classicladder
http://sites.google.com/site/classicladder


Here in this document, you will find all the known variables, and how you can
associates them to symbols names.
Also variables indexed usage is explained.
Latest part is a list of operators and functions usable in the arithmetic expressions


LIST OF KNOWN VARIABLES :
=========================

%Bxxx : Bit memory xx (boolean)
%Wxxx : Word memory xx (32 bits integer)
%Sxxx : System bit, see list below (boolean)
%SWxxx : System word, see list below (boolean)
%Txx.R : Timer xx running (boolean, user read only)
%Txx.D : Timer xx done (boolean, user read only)
%Txx.V : Timer xx current value (integer, user read only)
%Txx.P : Timer xx preset (integer)
%Mxx.R : Monostable xx running (boolean)
%Mxx.V : Monostable xx current value (integer, user read only)
%Mxx.P : Monostable xx preset (integer)
%Cxx.D : Counter xx done (boolean, user read only)
%Cxx.E : Counter xx empty overflow (boolean, user read only)
%Cxx.F : Counter xx full overflow (boolean, user read only)
%Cxx.V : Counter xx current value (integer)
%Cxx.P : Counter xx preset (integer)
%TMxx.Q : IEC Timer xx output (boolean, user read only)
%TMxx.V : IEC Timer xx current value (integer, user read only)
%TMxx.P : IEC Timer xx preset (integer)
%Rxxx.E : Register xx empty (boolean, user read only)
%Rxxx.F : Register xx full (boolean, user read only)
%Rxxx.I : Register xx input value (integer)
%Rxxx.O : Register xx output value (integer, user read only)
%Rxxx.S : Register xx nbr values currently stored (integer, user read only)
%Ixxx : Physical input xx (boolean) - see hardware -
%Qxxx : Physical output xx (boolean) - see hardware -
%IWxxx : Physical word input xx (integer) - see distributed hardware -
%QWxxx : Physical word output xx (integer) - see distributed hardware -
%QLEDx : Physical users leds (boolean) - %QLED0 = Green info led on ClassicLadderPLC
%Xxxx.A : Activity of step xx (sequential language)
%Xxxx.V : Time of activity in seconds of step xx (sequential language)


LIST OF SYSTEM VARIABLES :
==========================

%SW0: time in BCD
%SW1: date (year/month/day) in BCD
%SW2: week of day (0=sunday)
%S0: 1hz flash
%S1: 1hz flash inverted
%S2: 1 min flash
%S3: 1 min flash inverted
%S4: cold start
%S5: hot start
%S7: divide per zero error
%S8: out bit for SHL()/SHR()/ROL()/ROR() functions.
%S9: summer period
%S10: error of communication in the modbus master with Slave N°0.
... to %S19: error of communication in the modbus master with Slave N°9.
%S40: modem init & config is ok
%S41: modem connected

for-example, you can have on user led %QLED0 modem state with the following rung:
 |-[%S40]-+-[%S41]---[%S0]-+------(%QLED0)-|
 |        |                |               |
 |        +-NOT[%S41]------+               |
(led is on if modem init/config ok, and blinking during connection).


SYMBOLS :
=========

Each variable can be associated to a symbol name that can be then used instead of the
real variable name.
Partial symbol or complete symbol are possible.
A complete symbol corresponds directly to a real variable name:
ex: "InputK4" => %I17
A partial symbol is a part of a variable name without any attribute:
ex: "MyTimer" => %TM0 (that can not be used directly, but by adding
the attribute wanted: "MyTimer.Q" => %TM0.Q


INDEXED VARIABLES :
===================

In the arithmetic expressions and contacts/coils, variables indexed with another
one can be used.
Examples:
%W0[%W4] => if %W4 equals 23 it corresponds to %W23 !
%I0[%W20] => if %W20 equals 5 it corresponds to %I5 !
%B100[%W6] => if %W6 equals 56 it corresponds to %B156 !


CONSTANTS :
===========

Examples of constants usable in the arithmetic expressions:
=> 45 (decimal, per default)
=> $10 (hexadecimal, starting with $ character)
=> 'G' (character), between '').


LIST OF KNOWN OPERATORS IN ARITHMETIC EXPRESSIONS STRINGS :
===========================================================

& | ^   : and or xor
+ - * / % : plus minus mult div modulo
! : not

Functions available list (with variables or constants parameters) :
-------------------------------------------------------------------
ABS(a) : Absolute value
MINI(a,...,z) : Minimum value
MAXI(a,...,z) : Maximum value
AVG(a,...,z) : Average value
POW(a,b) : a power b
SHL(a,i) ROL(a,i) : Logic Shift or Rotate Left (out bit is stored in %SY8, for Rotate out bit is stored in bit 0)
SHR(a,i) ROR(a,i) : Logic Shift or Rotate Right (out bit is stored in %SY8, for Rotate out bit is stored in bit 31)


