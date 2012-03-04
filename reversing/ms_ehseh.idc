//Microsoft Visual C++ Win32 SEH/C++ EH info parser
//Version 3.0 2006.03.02 Igor Skochinsky <skochinsky@mail.ru>

#include <idc.idc>
#define __INCLUDED
#include <ms_rtti.idc>

static getEHRec()
{
  auto id;
  id = GetStrucIdByName("EHRegistrationNode");
  if (id==-1)
  {
    id = AddStruc(-1,"EHRegistrationNode");
    ForceDWMember(id, 0, "pNext");
    ForceDWMember(id, 4, "frameHandler");
    ForceDWMember(id, 8, "state");
  }
  return id;
}

static getEHRecCatch()
{
  auto id;
  id = GetStrucIdByName("EHRegistrationNodeCatch");
  if (id==-1)
  {
    id = AddStruc(-1,"EHRegistrationNodeCatch");
    ForceDWMember(id, 0, "SavedESP");
    ForceDWMember(id, 4, "pNext");
    ForceDWMember(id, 8, "frameHandler");
    ForceDWMember(id, 12, "state");
  }
  return id;
}

static CommentStackEH(start, hasESP, EHCookie, GSCookie)
{  
  if (hasESP)
    CommentStack(start,-16, "__$EHRec$", getEHRecCatch());
  else
    CommentStack(start,-12, "__$EHRec$", getEHRec());
  if (GSCookie)
    CommentStack(start,-GSCookie, "__$GSCookie$",-1);
  if (EHCookie)
    CommentStack(start,-EHCookie, "__$EHCookie$",-1);
}

/* from frame.obj
typedef struct _s_FuncInfo {
  unsigned int magicNumber;
  int maxState;
  const struct _s_UnwindMapEntry * pUnwindMap;
  unsigned int nTryBlocks;
  const struct _s_TryBlockMapEntry * pTryBlockMap;
  unsigned int nIPMapEntries;
  void * pIPtoStateMap;
  const struct _s_ESTypeList * pESTypeList;
} FuncInfo;
*/

//handler:
//  mov     eax, offset funcInfo
//  jmp     ___CxxFrameHandler
static ParseCxxHandler(func, handler, fixFunc)
{
  auto x, start, y, z, end, i, count, t, u, i2, cnt2, a, hasESP;
  auto EHCookieOffset, GSCookieOffset;
  start = func;
  x = handler;
  y = x;
  z = x;
  EHCookieOffset=0; GSCookieOffset=0;
  // 8B 54 24 08                       mov     edx, [esp+8]
  if (matchBytes(x,"8B5424088D02"))
    x = x+6;
    // 8D 02                             lea     eax, [edx]
  else if (matchBytes(x,"8B5424088D42"))
    x = x+7;
    // 8D 42 xx                          lea     eax, [edx+XXh]
  else if (matchBytes(x,"8B5424088D82"))
    x = x+10;
    // 8D 82 xx xx xx xx                 lea     eax, [edx+XXh]
  else {
    Message("Function at %08X not recognized as exception handler!\n",x);
    return;
  }
  //EH cookie check:
  // 8B 4A xx                          mov     ecx, [edx-XXh]
  //   OR
  // 8B 8A xx xx xx xx                 mov     ecx, [edx-XXh]
  // 33 C8                             xor     ecx, eax
  // E8 xx xx xx xx                    call    __security_check_cookie
  if (matchBytes(x,"8B4A??33C8E8"))
  {
    //byte argument
    EHCookieOffset = (~Byte(x+2)+1)&0xFF;
    EHCookieOffset = 12 + EHCookieOffset;
    x = x+10;
  }
  else if (matchBytes(x,"8B8A????????33C8E8"))
  {
    //dword argument
    EHCookieOffset = (~Dword(x+2)+1);
    EHCookieOffset = 12 + EHCookieOffset;
    x = x+13;
  }
  if (matchBytes(x,"83C0"))
    x = x + 3;
    // 8B 4A xx                          add     eax, XXh
  if (matchBytes(x,"8B4A??33C8E8"))
  {
    // 8B 4A xx                          mov     ecx, [edx-XXh]
    // 33 C8                             xor     ecx, eax
    // E8 xx xx xx xx                    call    __security_check_cookie
    GSCookieOffset = (~Byte(x+2)+1)&0xFF;
    GSCookieOffset = 12 + GSCookieOffset;
    x = x+10;
  }
  else if (matchBytes(x,"8B8A????????33C8E8"))
  {
    //dword argument
    GSCookieOffset = (~Dword(x+9)+1);
    GSCookieOffset = 12 + GSCookieOffset;
    x = x+13;
  }

  //Message("EH3: EH Cookie=%02X, GSCookie=%02X\n",EHCookieOffset, GSCookieOffset);

  if (Byte(x)==0xB8) {
    // 8B 4A xx xx xx                    mov     eax, offset FuncInfo
    x = Dword(x+1);
  }
  else {
    Message("\"mov eax, offset FuncInfo\" not found at offset %08X!\n",x);
    return;
  }
  if (Dword(x)-0x19930520>0xF) {
    Message("Magic is not 1993052Xh!\n");
    return;
  }
  Message(form("Detected function start at %08X\n",start));
  u = x; //FuncInfo;
  
  //parse unwind handlers
  count = Dword(u+4); //maxState
  i=0;
  x = Dword(u+8); //pUnwindMap
  while (i<count) {
    t = Dword(x+4); //unwind action address
    if (t<MAXADDR && t>y) y=t;   //find lowest
    if (t!=0 && t<z) z=t;   //find highest
    x = x+8;
    i = i+1;
  }
  if (y==0) {
    Message("All pointers are NULL!\n");
    return;
  }
  if (z>y)
  {
    Message("Something's very wrong!\n");
    return;
  }

  end = FindFuncEnd(y);
  if (end==BADADDR) {
    if (fixFunc) MakeUnkn(y, 1);
    if (BADADDR == FindFuncEnd(y))
    {
      Message(form("Can't find function end at 0x%08X\n",y));
      return;
    }
  }
  Message(form("Handlers block: %08X-%08X\n", z, y));
  if (GetFunctionFlags(start) == -1)
  {
    if (fixFunc) 
    { 
      MakeUnkn(start, 1);
      MakeCode(start);
      MakeFunction(start, BADADDR);
    }
    else
    {
      Message("There is no function defined at 0x%08X!\n", start);
      return;
    }
  }
  a = FindFuncEnd(start);
  Message("Function end: %08X\n", a);
  if (fixFunc) AnalyseArea(start,a);
  if (1)//(z>a) && ((z-a)>0x20))
  {
    //the handlers block is too far from the function end, make it a separate chunk
    if (fixFunc)
    {
      Message("Making separate handlers block\n");
      Unknown(z, y-z);
      MakeCode(z);
      MakeFunction(z,y);
      AnalyseArea(z,y);
      MakeCode(y);
      MakeFunction(y,BADADDR);
    }
    SetFunctionFlags(z, GetFunctionFlags(start) | FUNC_FRAME);
    SetFunctionCmt(z, form("Unwind handlers of %08X", start), 0);
  }
  else if (fixFunc)
  {
    Message("Merging handlers block with main function.\n");
    Unknown(start, y-start);
    MakeCode(start);
    MakeFunction(start,y);
    AnalyseArea(start,y);
  }
/*
typedef const struct _s_TryBlockMapEntry {
  int tryLow;                                  //00
  int tryHigh;                                 //04
  int catchHigh;                               //08
  int nCatches;                                //0C
  const struct _s_HandlerType * pHandlerArray; //10
} TryBlockMapEntry;

typedef const struct _s_HandlerType {
  unsigned int adjectives;          //00
  struct TypeDescriptor * pType;    //04
  int dispCatchObj;                 //08
  void * addressOfHandler;          //0C
}
*/  

  //parse catch blocks
  y = 0;
  z = 0x7FFFFFFF;
  i=0;
  count = Dword(u+12); //nTryBlocks
  x = Dword(u+16);     //pTryBlocksMap
  Message("%d try blocks\n",count);
  while (i<count) {    
    cnt2 = Dword(x+12);        //nCatches
    a = Dword(x+16);           //pHandlerArray
    i2 = 0;
    Message("   %d catches\n",cnt2);
    while (i2<cnt2)
    {
      t = Dword(a+12);
      //Message("    t=0x%08.8X\n",t);
      if (t!=BADADDR && t>y)
        y=t;      //find lowest
      if (z>t)
        z=t;      //find highest
      a = a+16;
      i2 = i2+1;
    }
    x = x+20;
    i = i+1;
  }
  hasESP = 0;
  if (count>0)
  {
    hasESP = 1;
    //Message("y=0x%08.8X, z=0x%08.8X\n",y,z);
    end = FindFuncEnd(y);
    if (end==BADADDR) {
      if (fixFunc) 
      { 
        MakeUnkn(y, 1);
        MakeCode(y);
      }
      if (BADADDR == FindFuncEnd(y))
      {
        Message(form("Can't find function end at 0x%08X\n",y));
        return;
      }
    }
    Message(form("Catch blocks: %08X-%08X\n", z, end));
    y = FindFuncEnd(start);
    if (y ==-1 || end > y)
    {
      if (fixFunc)
      {
        Message("Merging catch blocks with main function.\n");
        Unknown(start, end-start);
        MakeCode(start);
        MakeFunction(start,end);
        AnalyseArea(start,end);
      }
      else
        Message("Catch blocks are not inside the function!\n");
    }
  }

  //comment unwind handlers
  i=0;
  count = Dword(u+4); //maxState
  x = Dword(u+8); //pUnwindMap
  while (i<count) {
    t = Dword(x+4); //unwind action address
    if (t!=0)
      MakeComm(t, form("state %d -> %d",i, Dword(x)));
    x = x+8;
    i = i+1;
  }
  Parse_FuncInfo(u, 0);
  CommentStackEH(func, hasESP, EHCookieOffset, GSCookieOffset);
}

static fixCxx(s, doSEH, fixFunc) {
  auto x, start;
  start = s;
  if ((Word(start) != 0xA164) || (Dword(start+2)!=0)) {
    Message("Should start with \"move eax, large fs:0\"!\n");
    return;
  }
  if ( !doSEH && (Byte(start-10) == 0x55) && (Dword(start-9) == 0xFF6AEC8B))
  {
    //(ebp frame)
    //00: 55                   push    ebp
    //01: 8B EC                mov     ebp, esp
    //03: 6A FF                push    0FFFFFFFFh
    //05: 68 xx xx xx xx       push    loc_xxxxxxxx
    //0A: 64 A1 00 00 00 00    mov     eax, large fs:0
    //10: 50                   push    eax
    //11: 64 89 25 00 00 00 00 mov     large fs:0, esp
    start = start - 10;
    x = Dword(start+6);
    //Message("Match 1\n");
  }
  else if (!doSEH && (Word(start+9) == 0xFF6A) && (Byte(start+11)==0x68))
  {
    //00: 64 A1 00 00 00 00    mov     eax, large fs:0
    //06: xx xx xx
    //09: 6A FF                push    0FFFFFFFFh
    //0B: 68 xx xx xx xx       push    loc_xxxxxxxx
    //10: 50                   push    eax
    //
    x = Dword(start+12);
    //Message("Match 2\n");
  }
  else if (!doSEH && (Word(start-7) == 0xFF6A) && (Byte(start-5)==0x68))
  {
    //-7: 6A FF                push    0FFFFFFFFh
    //-5: 68 xx xx xx xx       push    loc_xxxxxxxx
    //00: 64 A1 00 00 00 00    mov     eax, large fs:0
    //06: 50                   push    eax
    //07: 64 89 25 00 00 00 00 mov     large fs:0, esp
    //
    x = Dword(start-4);
    start = start-7;
    //Message("Match 3\n");
  }
  else if (!doSEH && (Word(start+6) == 0xFF6A) && (Byte(start+8)==0x68))
  {
   //00: 64 A1 00 00 00 00    mov     eax, large fs:0
   //06: 6A FF                push    0FFFFFFFFh
   //08: 68 xx xx xx xx       push    loc_xxxxxxxx
   //0D: 50                   push    eax
   //0E: 64 89 25 00 00 00 00 mov     large fs:0, esp
    x = Dword(start+9);
    //Message("Match 4\n");
  }
  else if (doSEH && (Byte(start-5)==0x68) && (Byte(start-10)==0x68) && (Dword(start-15)==0x6AEC8B55))
  {
   //-15: 55                  push    ebp
   //-14: 8B EC               mov     ebp, esp
   //-12: 6A F?               push    0FFFFFFF?h
   //-10: 68 xx xx xx xx      push    offset __sehtable$_func1
   //-5 : 68 xx xx xx xx      push    offset _except_handlerx
   //00 : 64 A1 00 00 00 00   mov     eax, large fs:0
   x = Dword(start-9);
   //Message("Match 5\n");
   if (Byte(start-11) == 0xFF) //-1 = SEH3
     fixSEHFunc(start-15,x, 3, fixFunc);
   else if (Byte(start-11) == 0xFE) //-2 = SEH4
     fixSEHFunc(start-15,x, 4, fixFunc);
   else
     Message("Unknown SEH handler!\n");
   return;
  }
  else {
    //probably a custom handler
    //Message("\"push 0FFFFFFFFh; push offset loc\" not found!\n");
    return;
  }
  Message(form("Fixing function at 0x%08X\n",start));
  ParseCxxHandler(start, x, fixFunc);
}

static doEHProlog(name,fixFunc)
{
  auto i,s,a;
  a=LocByName(name);
  if (a==BADADDR)
    return;
  Message("%s = %08X\n",name,a);  
  i=RfirstB(a);
  while(i!=BADADDR)
  {
    Message("- %08X - ",i);

    //  -5: mov  eax, offset loc_XXXXXX
    //   0: call __EH_prolog
    if (Byte(i-5)==0xB8)
      ParseCxxHandler(i-5, Dword(i-4),fixFunc);
    else
    {
      Message(form("No mov eax, offset loc_XXXXXX at %08X!!!\n",i-5));
      return;
    }

    if (SetFunctionFlags(i,GetFunctionFlags(i) | FUNC_FRAME))
    {
      MakeFrame(i,GetFrameLvarSize(i), 4, GetFrameArgsSize(i));
      if (fixFunc) AnalyseArea(i, FindFuncEnd(i)+1);
      Message("OK\n");
    }
    else
      Message("Error\n");
    i=RnextB(a,i);
  }
}

static doEHPrologs(name, fixFunc)
{
  doEHProlog("j"+name,fixFunc);
  doEHProlog("j_"+name,fixFunc);
  doEHProlog(name,fixFunc);
  doEHProlog("_"+name,fixFunc);
}

static fixEHPrologs(fixFunc)
{
  doEHPrologs("_EH_prolog",fixFunc);
  doEHPrologs("_EH_prolog3",fixFunc);
  doEHPrologs("_EH_prolog3_catch",fixFunc);
  doEHPrologs("_EH_prolog3_GS",fixFunc);
  doEHPrologs("_EH_prolog3_catch_GS",fixFunc);
}

static isInCodeSeg(a)
{
  if (SegName(a)==".text")
    return 1;
  else
    return 0;
}

//check a scopetable entry
static checkEntry(a,i,ver)
{
  auto x;
  x = Dword(a);
  //EnclosingLevel should be negative or less than i
  if (x&0x80000000)
  {
    if (ver==3 && x!=0xFFFFFFFF)
      return 0;
    if (ver==4 && x!=0xFFFFFFFE)
      return 0;
  }
  else if (x>=i)
    return 0;

  x = Dword(a+4);
  if ((x!=0) && !isInCodeSeg(x)) //filter should be zero or point to the code
    return 0;
  x = Dword(a+8);
  if (!isInCodeSeg(x)) //handler should point to the code
    return 0;

  //check if there are xref to fields (i.e. after the end of the scopetable)
  if (((ver!=3)||(i>0)) && isRef(GetFlags(a)))
      return 0;
  if (isRef(GetFlags(a+4)) || isRef(GetFlags(a+8)))
      return 0;
  return 1;
}

//check if there's a valid scopetable and calculate number of entries in it
static checkScopeTable(a, ver)
{
  auto i,k;
  if (ver==4)
  {
    k = Dword(a);
    if ((k&0x80000000)==0) //first field should be negative
      return 0;
    if ((k!=0xFFFFFFFE) && (k&3)!=0) //GS cookie offset should be -2 or dword-aligned
      return 0;
    k = Dword(a+8);
    if ((k&0x80000000)==0) //offset should be negative
      return 0;
    if ((k&3)!=0) //EH cookie offset should be dword-aligned
      return 0;
    a = a+16; //move to the scope entries list
  }

  i = 0;
  while (checkEntry(a,i,ver))
  {
    i = i+1;
    a = a+12;
  }
  return i;
}

/*
struct _EH4_EXCEPTION_REGISTRATION_RECORD {
	void* SavedESP;
	_EXCEPTION_POINTERS* ExceptionPointers;
	_EXCEPTION_REGISTRATION_RECORD* Next;
	enum _EXCEPTION_DISPOSITION (*Handler)(_EXCEPTION_RECORD*, void*, _CONTEXT*, void*);
	DWORD EncodedScopeTable;
	unsigned long TryLevel;
};
*/

static getSEHRec()
{
  auto id;
  id = GetStrucIdByName("SEHRegistrationNode");
  if (id==-1)
  {
    id = AddStruc(-1,"SEHRegistrationNode");
    ForceDWMember(id, 0, "SavedESP");
    ForceDWMember(id, 4, "ExceptionPointers");
    ForceDWMember(id, 8, "Next");
    ForceDWMember(id, 12, "Handler");
    ForceDWMember(id, 16, "EncodedScopeTable");
    ForceDWMember(id, 20, "TryLevel");
  }
  return id;
}

static CommentStackSEH(start, scopetable)
{
  auto x;
  CommentStack(start,-24, "__$SEHRec$", getSEHRec());
  if (scopetable)
  {
    x = Dword(scopetable);
    if (x!=-2)
      CommentStack(start,x, "__$GSCookie$", -1);
    x = Dword(scopetable+8);
    CommentStack(start,x, "__$EHCookie$", -1);
  }
}

static fixSEHFunc(func, scopetable, ver, fixFunc)
{
  auto k,i,t,u,x,y,z,hasESP,end;
  k = checkScopeTable(scopetable, ver);
  if (k==0)
  {
    Message("Bad scopetable\n");
    return;
  }
  Message("function: %08X, scopetable: %08X (%d entries)\n", func, scopetable, k);
  x = scopetable;
  if (ver==4) x = x+16;

  //parse the scopetable!
  y = 0;
  z = 0x7FFFFFFF;
  i = 0;
  hasESP = 0;
  while (i<k) {       
    t = Dword(x+4);
    if (t) {
      hasESP=1;
      if (t>y) y=t; //find lowest
      if (z>t) z=t; //find highest
      //Message("t=0x%08.8X\n",t);
      //check the code just before, it could be jump to the end of try
      if (Byte(t-2)==0xEB)
        t = getRelJmpTarget(t-2);
      else if (Byte(t-5)==0xE9)
        t = getRelJmpTarget(t-5);
      //Message("t=0x%08.8X\n",t);
      if (t>y) y=t; //find lowest
      if (z>t) z=t; //find highest
    }
    t = Dword(x+8);
      //check the code just before, it could be jump to the end of try
    if (t>y) y=t; //find lowest
    if (z>t) z=t; //find highest
    //Message("t=0x%08.8X\n",t);
      if (Byte(t-2)==0xEB)
        t = getRelJmpTarget(t-2);
      else if (Byte(t-5)==0xE9)
        t = getRelJmpTarget(t-5);
    //Message("t=0x%08.8X\n",t);
    if (t>y) y=t; //find lowest
    if (z>t) z=t; //find highest
    x = x+12;
    i = i+1;
  }
  

  //Message("y=0x%08.8X, z=0x%08.8X\n",y,z);
  if (1)
  {
    end = FindFuncEnd(y);
    if (end==BADADDR) {
      if (fixFunc)
      {
        MakeUnkn(y, 1);
        MakeCode(y);
      }
      if (BADADDR == FindFuncEnd(y))
      {
        Message(form("Can't find function end at 0x%08X\n",y));
        return;
      }
    }
    //Message(form("Except blocks: %08X-%08X\n", z, end));
    z = FindFuncEnd(func);
    if (z ==-1 || end > z && fixFunc)
    {
      //Message("Merging except blocks with main function.\n");
      Unknown(func, end-func);
      MakeCode(func);
      MakeFunction(func,end);
      AnalyseArea(func,end);
    }
  }

  //walk once more and fix finally entries
  x = scopetable;
  if (ver==4) x = x+16;
  i = 0;
  while (fixFunc && i<k) {
    if (Dword(x+4)==0 && Dword(x+8)==y)
    {
      //the last handler is a finally handler
      //check that it ends with a ret, call or jmp
      z = FindFuncEnd(y);
      if (z!=BADADDR && 
          !(Byte(z-1)==0xC3 || Byte(z-5)==0xE9 || Byte(z-5)==0xE8 || 
            Byte(z-2)==0xEB || Byte(z-1)==0xCC || Word(z-6)==0x15FF) )
      {        
        //we need to add the following funclet to our function
        end = FindFuncEnd(z);
        if (end!=BADADDR)
        {
          Unknown(z, end-z);
          MakeCode(z);
          SetFunctionEnd(func,end);
        }
      }
    }
    x = x+12;
    i = i+1;
  }

  //comment the table and handlers
  x = scopetable;
  ExtLinA(x,0,form("; SEH scopetable for %08X",func));
  if (ver==4) 
  {
    OffCmt(x,"GSCookieOffset");
    OffCmt(x+4,"GSCookieXOROffset");
    OffCmt(x+8,"EHCookieOffset");
    OffCmt(x+12,"EHCookieXOROffset");
    x = x+16;
    CommentStackSEH(func,scopetable);
  }
  else
    CommentStackSEH(func,0);
  i = 0;
  while (i<k) {
    ForceDword(x);
    SoftOff(x+4);
    SoftOff(x+8);
    MakeComm(x, form("try block %d, enclosed by %d",i, Dword(x)));
    t = Dword(x+4); //exception filter
    if (t!=0)
      ExtLinA(t,0,form("; __except() filter for try block %d",i));
    u = Dword(x+8);
    if (t!=0)
      ExtLinA(u,0,form("; __except {} handler for try block %d",i));
    else
      ExtLinA(u,0,form("; __finally {} handler for try block %d",i));
    x = x+12;
    i = i+1;
  }
}

static doSEHProlog(name, ver, fixFunc)
{
  auto i,s,locals,scopetable,k,l,func,a;
  a=LocByName(name);
  if (a==BADADDR)
    return;
  Message("%s = %08X\n",name,a);  
  i=RfirstB(a);
  while(i!=BADADDR)
  {
    Message("- %08X - ",i);

    // -10 68 xx xx xx xx   push    xx
    //   or
    // -7  6A xx            push    xx
    // -5  68 xx xx xx xx   push    OFFSET __sehtable$_func
    // 0   e8 00 00 00 00   call    __SEH_prolog
    //   
    //
    locals = -1; scopetable=0;
    if (Byte(i-5)==0x68) 
    {      
      scopetable = Dword(i-4);
      if (Byte(i-7)==0x6A)
      {
        func = i-7;
        locals = Byte(func+1);
      }
      else if (Byte(i-10)==0x68)
      {
        func = i-10;
        locals = Dword(func+1);
      }
      if (GetFunctionFlags(func)==-1 && fixFunc)
      {
        MakeUnkn(func, 1);
        MakeCode(func);
        MakeFunction(func, BADADDR);
      }
      if (SetFunctionFlags(func,GetFunctionFlags(func)|FUNC_FRAME))
      {
        MakeFrame(func, GetFrameLvarSize(func), 4, GetFrameArgsSize(func));
        fixSEHFunc(func, scopetable, ver, fixFunc);
        Message("OK\n");
      }
      else
        Message("Error\n");
    }  
    i=RnextB(a,i);
  }
}

static doSEHPrologs(name, ver, fixFunc)
{
  doSEHProlog("j"+name, ver, fixFunc);
  doSEHProlog("j_"+name, ver, fixFunc);
  doSEHProlog(name, ver, fixFunc);
  doSEHProlog("_"+name, ver, fixFunc);
}

static fixSEHPrologs(fixFunc)
{
  doSEHPrologs("_SEH_prolog",3, fixFunc);
  doSEHPrologs("__SEH_prolog",3, fixFunc);
  doSEHPrologs("_SEH_prolog4",4, fixFunc);
  doSEHPrologs("__SEH_prolog4",4, fixFunc);
  doSEHPrologs("_SEH_prolog4_GS",4, fixFunc); 
  doSEHPrologs("__SEH_prolog4_GS",4, fixFunc); 
}

static findFunc(name)
{
  auto a;
  a = LocByName("j_"+name); 
  if (a==BADADDR)
    a = LocByName(name);
  return a;
}

static doSEH(fixFunc)
{
  auto start, a;
  start = 0;
  while (1) {
    //mov eax, large fs:0
    start = FindBinary(start+1, 3, "64 A1 00 00 00 00");
    if (start==BADADDR)
      break;
    fixCxx(start,1,fixFunc);
  }
  fixSEHPrologs(fixFunc);
}

static doEH(fixFunc)
{
  auto start, a;
  start = 0;
  while (1) {
    //mov eax, large fs:0
    start = FindBinary(start+1, 3, "64 A1 00 00 00 00");
    if (start==BADADDR)
      break;
    fixCxx(start,0,fixFunc);
  }
  fixEHPrologs(fixFunc);
}

static main(void)
{
  auto seh, fixseh, eh, fixeh;
  seh = AskYN(1, "Do you wish to parse all Win32 SEH handlers?");
  if (seh==-1) return;
  if (seh) {
    fixseh = AskYN(1, "Do you wish to fix function boundaries as needed?");
    if (fixseh==-1) return;
  }
  eh = AskYN(1, "Do you wish to parse all C++ EH handlers?");
  if (eh==-1) return;
  if (eh) {
    fixeh = AskYN(1, "Do you wish to fix function boundaries as needed?");
    if (fixeh==-1) return;
  }
  if (seh) doSEH(fixseh);
  if (eh) doEH(fixeh);
  //fixCxx(ScreenEA());
}
