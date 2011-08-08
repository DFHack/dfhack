#include "functioncall.h"

#ifdef LINUX_BUILD
#define __F_TDEF(CONV,CONV_,tag) typedef int(__attribute__ ( (CONV_) ) *F_TYPE##CONV##tag)
#else
#define __F_TDEF(CONV,CONV_,tag) typedef int(__ ## CONV_ *F_TYPE##CONV##tag)
#endif

#define __F_T(CONV,tag) F_TYPE##CONV##tag
#define __F_TYPEDEFS(CONV,CONV_)	__F_TDEF(CONV,CONV_,1)(int);\
	__F_TDEF(CONV,CONV_,2)(int,int);\
	__F_TDEF(CONV,CONV_,3)(int,int,int);\
	__F_TDEF(CONV,CONV_,4)(int,int,int,int);\
	__F_TDEF(CONV,CONV_,5)(int,int,int,int,int);\
	__F_TDEF(CONV,CONV_,6)(int,int,int,int,int,int);\
	__F_TDEF(CONV,CONV_,7)(int,int,int,int,int,int,int)


#define __FCALL(CONV,CONV_) if(conv==CONV)\
	{	\
		if(count==1)\
			ret= (reinterpret_cast<__F_T(CONV,1)>(f))\
				   (arguments[0]);\
		else if(count==2)\
			ret= (reinterpret_cast<__F_T(CONV,2)>(f))\
				   (arguments[0],arguments[1]);\
		else if(count==3)\
			ret= (reinterpret_cast<__F_T(CONV,3)>(f))\
				   (arguments[0],arguments[1],arguments[2]);\
		else if(count==4)\
			ret= (reinterpret_cast<__F_T(CONV,4)>(f))\
			       (arguments[0],arguments[1],arguments[2],arguments[3]);\
		else if(count==5)\
			ret= (reinterpret_cast<__F_T(CONV,5)>(f))\
			       (arguments[0],arguments[1],arguments[2],arguments[3],arguments[4]);\
		else if(count==6)\
			ret= (reinterpret_cast<__F_T(CONV,6)>(f))\
			       (arguments[0],arguments[1],arguments[2],arguments[3],arguments[4],arguments[5]);\
		else if(count==7)\
			ret= (reinterpret_cast<__F_T(CONV,7)>(f))\
			       (arguments[0],arguments[1],arguments[2],arguments[3],arguments[4],arguments[5],arguments[6]);\
	}

#define __FCALLEX(CONV,CONV_) if(conv==CONV)\
	{	if(count==1)	 {__F_T(CONV,1) tmp_F=reinterpret_cast<__F_T(CONV,1)>(f); return tmp_F(arguments[0]);}\
		else if(count==2){__F_T(CONV,2) tmp_F=reinterpret_cast<__F_T(CONV,2)>(f); return tmp_F(arguments[0],arguments[1]);}\
		else if(count==3){__F_T(CONV,3) tmp_F=reinterpret_cast<__F_T(CONV,3)>(f); return tmp_F(arguments[0],arguments[1],arguments[2]);}\
		else if(count==4){__F_T(CONV,4) tmp_F=reinterpret_cast<__F_T(CONV,4)>(f); return tmp_F(arguments[0],arguments[1],arguments[2],arguments[3]);}\
		else if(count==5){__F_T(CONV,5) tmp_F=reinterpret_cast<__F_T(CONV,5)>(f); return tmp_F(arguments[0],arguments[1],arguments[2],arguments[3],arguments[4]);}\
		else if(count==6){__F_T(CONV,6) tmp_F=reinterpret_cast<__F_T(CONV,6)>(f); return tmp_F(arguments[0],arguments[1],arguments[2],arguments[3],arguments[4],arguments[5]);}\
		else if(count==7){__F_T(CONV,7) tmp_F=reinterpret_cast<__F_T(CONV,7)>(f); return tmp_F(arguments[0],arguments[1],arguments[2],arguments[3],arguments[4],arguments[5],arguments[6]);}\
	}
		/*else if(count==8)\
			ret= (reinterpret_cast<__F_T(CONV,8)>(f))\
			       (arguments[0],arguments[1],arguments[2],arguments[3],arguments[4],arguments[5],arguments[6],arguments[7]);\
		else if(count==9)\
			ret= (reinterpret_cast<int (CONV_*)(int,int,int,int,int\
												,int,int,int,int)>(f))\
			       (arguments[0],arguments[1],arguments[2],arguments[3],arguments[4],arguments[5],arguments[6],arguments[7],arguments[8]);\
		else if(count==10)\
			ret= (reinterpret_cast<int (CONV_*)(int,int,int,int,int\
												,int,int,int,int,int)>(f))\
			       (arguments[0],arguments[1],arguments[2],arguments[3],arguments[4],arguments[5],arguments[6],arguments[7],arguments[8],arguments[9]);}*/
	

/*int FunctionCaller::CallF(size_t count,callconv conv,void* f,const vector<int> &arguments)//more complex but not more error safe
{
	__F_TYPEDEFS(STD_CALL,__stdcall);
	__F_TYPEDEFS(FAST_CALL,__fastcall);
	__F_TYPEDEFS(THIS_CALL,__thiscall);
	__F_TYPEDEFS(CDECL_CALL,__cdecl);
	{
	__FCALLEX(STD_CALL,__stdcall);
	__FCALLEX(FAST_CALL,__fastcall);
	__FCALLEX(THIS_CALL,__thiscall);
	__FCALLEX(CDECL_CALL,__cdecl);
	}
}*/
int FunctionCaller::CallFunction(size_t func_ptr,callconv conv,const vector<int> &arguments)
{
	//nasty nasty code...
#ifdef LINUX_BUILD	//quick fix
	if(conv==THIS_CALL)
		conv=STD_CALL;
#endif
	void* f= reinterpret_cast<void*>(func_ptr+base_);
	size_t count=arguments.size();
	if(count==0)
		return (reinterpret_cast<int (*)()>(f))(); //does not matter how we call it...
	int ret=0;
	//typedefs
	__F_TYPEDEFS(STD_CALL,stdcall);
	__F_TYPEDEFS(FAST_CALL,fastcall);
	__F_TYPEDEFS(THIS_CALL,thiscall);
	__F_TYPEDEFS(CDECL_CALL,cdecl);
	//calls
	__FCALL(STD_CALL,stdcall);
	__FCALL(FAST_CALL,fastcall);
	__FCALL(THIS_CALL,thiscall);
	__FCALL(CDECL_CALL,cdecl);
	return -1; //incorect type. Should probably throw...
	//return CallF(count,conv,f,arguments);
	/*//testing part{  worked some time ago..., put where DFHack::Core is accesible
	c->Suspend();
	FunctionCaller caller(c->p->getBase()); 
	std::vector <int> args;
	args.push_back((size_t)"Hello world");
	args.push_back(4);
	args.push_back(4);
	args.push_back(0);
	dfprint  mprint=(dfprint)(0x27F030+c->p->getBase());
	mprint("Hello world",4,4,0);
	//caller.CallFunction((0x27F030),FunctionCaller::THIS_CALL,args);
	c->Resume();
	return CR_OK;
	//}end testing*/
}
#undef __FCALL
#undef __FCALLEX
#undef __F_TYPEDEFS
#undef __F_T
