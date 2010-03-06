#include <idc.idc>
#include <vtable.idc>
#include <ms_rtti.idc>

static GetAsciizStr(x)
{
  auto s,c;
  s = "";
  while (c=Byte(x))
  {
    s = form("%s%c",s,c);
    x = x+1;
  }
  return s;
}

// ??1?$CEventingNode@VCMsgrAppInfoImpl@@PAUIMsgrUser@@ABUtagVARIANT@@@@UAE@XZ
// ??_G CWin32Heap@ATL @@UAEPAXI@Z
// ATL::CWin32Heap::`scalar deleting destructor'(uint)
// .?AV?$CEventingNode@VCMsgrAppInfoImpl@@PAUIMsgrUser@@ABUtagVARIANT@@@@
// ??_7?$CEventingNode@VCMsgrAppInfoImpl@@PAUIMsgrUser@@ABUtagVARIANT@@@@6B@
// ??_G?$CEventingNode@VCMsgrAppInfoImpl@@PAUIMsgrUser@@ABUtagVARIANT@@@@@@UAEPAXI@Z

#define SN_constructor 1
#define SN_destructor  2
#define SN_vdestructor 3
#define SN_scalardtr   4
#define SN_vectordtr   5

static MakeSpecialName(name, type, adj)
{
  auto basename;
  //.?AUA@@ = typeid(struct A)
  //basename = A@@
  basename = substr(name,4,-1);
  if (type==SN_constructor)
  {
    //??0A@@QAE@XZ = public: __thiscall A::A(void)
    if (adj==0)
      return "??0"+basename+"QAE@XZ";
    else
      return "??0"+basename+"W"+MangleNumber(adj)+"AE@XZ";
  }
  else if (type==SN_destructor)
  {
    //??1A@@QAE@XZ = "public: __thiscall A::~A(void)"
    if (adj==0)
      return "??1"+basename+"QAE@XZ";
    else
      return "??1"+basename+"W"+MangleNumber(adj)+"AE@XZ";
  }
  else if (type==SN_vdestructor)
  {
    //??1A@@UAE@XZ = public: virtual __thiscall A::~A(void)
    if (adj==0)
      return "??1"+basename+"UAE@XZ";
    else
      return "??1"+basename+"W"+MangleNumber(adj)+"AE@XZ";
  }
  else if (type==SN_scalardtr) //
  {
    //??_GA@@UAEPAXI@Z = public: virtual void * __thiscall A::`scalar deleting destructor'(unsigned int)
    if (adj==0)
      return "??_G"+basename+"UAEPAXI@Z";
    else
      return "??_G"+basename+"W"+MangleNumber(adj)+"AEPAXI@Z";
  }
  else if (type==SN_vectordtr)
  {
    //.?AUA@@ = typeid(struct A)
    //??_EA@@UAEPAXI@Z = public: virtual void * __thiscall A::`vector deleting destructor'(unsigned int)
    if (adj==0)
      return "??_E"+basename+"QAEPAXI@Z";
    else
      return "??_E"+basename+"W"+MangleNumber(adj)+"AEPAXI@Z";
  }
}

static DumpNestedClass2(x, indent, contained, f)
{
  auto indent_str,i,a,n,p,s,off;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
  i=0;
  //indent=indent+1;
  a = x;
  while(i<contained)
  {
    p = Dword(a);
    off = Dword(p+8);
    s = form("%.4X: ",off);
    //Message("%s%s%s\n", s, indent_str, GetClassName(p));
    fprintf(f, form("%s%s%s\n",s,indent_str,GetClassName(p)));
    n = Dword(p+4);
    if (n>0) //check numContainedBases
      DumpNestedClass2(a+4, indent+1, n, f); //nested classes following
    a=a+4*(n+1);
    i=i+n+1;
  }
}

static Parse_CHD2(x, indent, f)
{
  auto indent_str,i,a,n,p,s,off;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
  a = Dword(x+4);
  if ((a&3)==1)
    p = "(MI)";
  else if ((a&3)==2)
    p = "(VI)";
  else if ((a&3)==3)
    p = "(MI VI)";
  else
    p="(SI)";

  fprintf(f, form("%s%s\n",indent_str,p));
  a=Dword(x+12);
  n=Dword(x+8);
  DumpNestedClass2(a, indent, n, f);
}

static GetTypeName2(col)
{
  auto x, s, c;
  //Message("GetTypeName2(%X)\n",col)
  x = Dword(col+12);
  if ((!x) || (x==BADADDR)) return "";
  return GetAsciizStr(x+8);
}

static GetVtblName2(col)
{
  auto i, s, s2;
  s = GetTypeName2(col);
  i = Dword(col+16); //CHD
  i = Dword(i+4);  //Attributes
  if ((i&3)==0 && Dword(col+4)==0)
  { 
    //Single inheritance, so we don't need to worry about duplicate names (several vtables)
    s=substr(s,4,-1);
    return "??_7"+s+"6B@";
  }
  else //if ((i&3)==1) //multiple inheritance
  { 
    s2 = GetVtableClass(col);
    s2 = substr(s2,4,-1);
    s  = substr(s,4,-1);
    s = s+"6B"+s2+"@";
    return "??_7"+s;
  }
  return "";
}

//check if Dword(vtbl-4) points to typeinfo record and extract the type name from it
static IsValidCOL(col)
{
  auto x, s, c;
  x = Dword(col+12);
  if ((!x) || (x==BADADDR)) return "";
  x = Dword(x+8);
  if ((x&0xFFFFFF) == 0x413F2E) //.?A
    return 1;
  else
    return 0;
}

static funcStart(ea)
{
  if (GetFunctionFlags(ea) == -1)
    return -1;

  if ((GetFlags(ea)&FF_FUNC)!=0)
    return ea;
  else
    return PrevFunction(ea);
}

// Add ea to "Sorted Address List"
static AddAddr(ea)
{
  auto id, idx, val;
  
  if ( (id = GetArrayId("AddrList")) == -1 )
  {
    id  = CreateArray("AddrList");
    SetArrayLong(id, 0, ea);
    return;
  }

  for ( idx = GetFirstIndex(AR_LONG, id); idx != -1; idx = GetNextIndex(AR_LONG, id, idx) )
  {
    val = GetArrayElement(AR_LONG, id, idx);
    if ( val == ea )
      return;
    if ( val > ea )    // InSort
    {
      for ( ; idx != -1; idx = GetNextIndex(AR_LONG, id, idx) )
      {
        val = GetArrayElement(AR_LONG, id, idx);
        SetArrayLong(id, idx, ea);
        ea = val;
      }
    }
  }
  SetArrayLong(id, GetLastIndex(AR_LONG, id) + 1, ea);
}
static getArraySize(id)
{
  auto idx, count;
  count = 0;
  for ( idx = GetFirstIndex(AR_LONG, id); idx != -1; idx = GetNextIndex(AR_LONG, id, idx) )
  {
    count++;
  }
  return count;
}

static doAddrList(name,f)
{
  auto idx, id, val, ctr, dtr;
  id = GetArrayId("AddrList");
  ctr = 0; dtr = 0;
  if ( name!=0 && id != -1 )
  {
    Message("refcount:%d\n",getArraySize(id));
    if (getArraySize(id)!=2)
      return;
    for ( idx = GetFirstIndex(AR_LONG, id); idx != -1; idx = GetNextIndex(AR_LONG, id, idx) )
    {
      val = GetArrayElement(AR_LONG, id, idx);
      if (Byte(val)==0xE9)
        val = getRelJmpTarget(val);
      if ((substr(Name(val),0,3)=="??1"))
        dtr = val;
      else
        ctr = val;
    }
  }
  if (ctr!=0 && dtr!=0)
  {
    Message("  constructor at %a\n",ctr);
    fprintf(f, "  constructor: %08.8Xh\n",ctr);
    MakeName(ctr, MakeSpecialName(name,SN_constructor,0));
  }
  DeleteArray(GetArrayId("AddrList"));
}

//check if there's a vtable at a and dump into to f
//returns position after the end of vtable
static DoVtable(a,f)
{
  auto x,y,s,p,q,i,name;

  //check if it looks like a vtable
  y = GetVtableSize(a);
  if (y==0)
    return a+4;
  s = form("%08.8Xh: possible vtable (%d methods)\n", a, y);
  Message(s);
  fprintf(f,s);

  //check if it's named as a vtable
  name = Name(a);
  if (substr(name,0,4)!="??_7") name=0;
 
  x = Dword(a-4);  
  //otherwise try to get it from RTTI
  if (IsValidCOL(x))
  {
    Parse_Vtable(a);
    if (name==0)
      name = GetVtblName2(x);
    //only output object tree for main vtable
    if (Dword(x+4)==0)
      Parse_CHD2(Dword(x+16),0,f);
    MakeName(a, name);
  }
  if (name!=0)
  {
    s = Demangle(name, 0x00004006);
    Message("%s\n",s);
    fprintf(f, "%s\n", s);
    //convert vtable name into typeinfo name
    name = ".?AV"+substr(name, 4, strstr(name,"@@6B")+2);
  }
  {
    DeleteArray(GetArrayId("AddrList"));
    Message("  referencing functions: \n");
    fprintf(f,"  referencing functions: \n");
    q = 0; i = 1;
    for ( x=DfirstB(a); x != BADADDR; x=DnextB(a,x) )
    {
       p = funcStart(x);
       if (p!=-1)
       { 
         if (q==p) 
           i++;
         else
         {           
           if (q) {
             if (i>1) s = form("  %a (%d times)",q,i);
             else s = form("  %a",q);
             //if (strstr(Name(p),"sub_")!=0 && strstr(Name(p),"j_sub_")!=0)
             if (hasName(GetFlags(q)))
               s = s+" ("+Demangle(Name(q),8)+")";
             s = s+"\n";
             Message(s);fprintf(f,s);
             AddAddr(q);
           }
           i = 1;
           q = p;
         }
       }
    }
    if (q)
    {           
       if (i>1) s = form("  %a (%d times)",q,i);
       else s = form("  %a",q);
       if (hasName(GetFlags(q)))
         s = s+" ("+Demangle(Name(q),8)+")";
       s = s+"\n";
       Message(s);fprintf(f,s);
       AddAddr(q);
    }
    
    x = a;
    while (y>0)
    {
      p = Dword(x);
      if (GetFunctionFlags(p) == -1)
      {
        MakeCode(p);
        MakeFunction(p, BADADDR);
      }
      checkSDD(p,name,a,0,f);
      y--;
      x = x+4;
    }
    doAddrList(name,f);
    Message("\n");
    fprintf(f,"\n");
  }
  return x;
}

static scan_for_vtables(void)
{
  auto rmin, rmax, cmin, cmax, s, a, x, y,f;
  s = FirstSeg();
  f = fopen("objtree.txt","w");
  rmin = 0; rmax = 0;
  while (s!=BADADDR)
  {
    if (SegName(s)==".rdata")
    {
      rmin = s;
      rmax = NextSeg(s);
    }
    else if (SegName(s)==".text")
    {
      cmin = s;
      cmax = NextSeg(s);
    }
    s = NextSeg(s);
  }
  if (rmin==0) {rmin=cmin; rmax=cmax;}
  a = rmin;
  Message(".rdata: %08.8Xh - %08.8Xh, .text %08.8Xh - %08.8Xh\n", rmin, rmax, cmin, cmax);
  while (a<rmax)
  {
    x = Dword(a);
    if (x>=cmin && x<cmax) //methods should reside in .text
    {
       a = DoVtable(a,f);
    }
    else
      a = a + 4;
  }
  Message("Done\n");
  fclose(f);
}

//check for `scalar deleting destructor'
static checkSDD(x,name,vtable,gate,f)
{
  auto a,s,t;
  //Message("checking function at %a\n",x);

  t = 0; a = BADADDR;

  if ((name!=0) && (substr(Name(x),0,3)=="??_") && (strstr(Name(x),substr(name,4,-1))==4))
    name=0; //it's already named

  if (Byte(x)==0xE9 || Byte(x)==0xEB) {
    //E9 xx xx xx xx   jmp   xxxxxxx
    return checkSDD(getRelJmpTarget(x),name,vtable,1,f);
  }
  else if (matchBytes(x,"83E9??E9")) {
    //thunk
    //83 E9 xx        sub     ecx, xx
    //E9 xx xx xx xx  jmp     class::`scalar deleting destructor'(uint)
    a = getRelJmpTarget(x+3);
    Message("  %a: thunk to %a\n",x,a);
    t = checkSDD(a,name,vtable,0,f);
    if (t && name!=0)
    {
      //rename this function as a thunk
      MakeName(x, MakeSpecialName(name,t,Byte(x+2)));
    }
    return t;
  }
  else if (matchBytes(x,"81E9????????E9")) {
    //thunk
    //81 E9 xx xx xx xx        sub     ecx, xxxxxxxx
    //E9 xx xx xx xx           jmp     class::`scalar deleting destructor'(uint)
    a = getRelJmpTarget(x+6);
    Message("  %a: thunk to %a\n",x,a);
    t = checkSDD(a,name,vtable,0,f);
    if (t && name!=0)
    {
      //rename this function as a thunk
      MakeName(x, MakeSpecialName(name,t,Dword(x+2)));
    }
    return t;
  }
  else if (matchBytes(x,"568BF1E8????????F64424080174") && matchBytes(x+15+Byte(x+14),"8BC65EC20400"))
  {
    //56                             push    esi
    //8B F1                          mov     esi, ecx
    //E8 xx xx xx xx                 call    class::~class()
    //F6 44 24 08 01                 test    [esp+arg_0], 1
    //74 07                          jz      short @@no_free
    //56                             push    esi
    //                               
    //                           call operator delete();
    
    //   @@no_free:
    //8B C6                          mov     eax, esi
    //5E                             pop     esi
    //C2 04 00                       retn    4

    t = SN_scalardtr;
    a = getRelCallTarget(x+3);
    if (gate && Byte(a)==0xE9)
    {
      //E9 xx xx xx xx   jmp   xxxxxxx
      a = getRelJmpTarget(a);
    }
  }
  else if (matchBytes(x,"568BF1FF15????????F64424080174") && matchBytes(x+16+Byte(x+15),"8BC65EC20400"))
  {
    //56                             push    esi
    //8B F1                          mov     esi, ecx
    //FF 15 xx xx xx xx              call    class::~class() //dllimport
    //F6 44 24 08 01                 test    [esp+arg_0], 1
    //74 07                          jz      short @@no_free
    //56                             push    esi
    //                               
    //                           call operator delete();
    
    //   @@no_free:
    //8B C6                          mov     eax, esi
    //5E                             pop     esi
    //C2 04 00                       retn    4

    t = SN_scalardtr;
    /*a = getRelCallTarget(x+3);
    if (gate && Byte(a)==0xE9)
    {
      //E9 xx xx xx xx   jmp   xxxxxxx
      a = getRelJmpTarget(a);
    }*/
  }
  else if (matchBytes(x,"558BEC51894DFC8B4DFCE8????????8B450883E00185C0740C8B4DFC51E8????????83C4048B45FC8BE55DC20400") ||
           matchBytes(x,"558BEC51894DFC8B4DFCE8????????8B450883E00185C074098B4DFC51E8????????8B45FC8BE55DC20400"))
  {
    //55                             push    ebp
    //8B EC                          mov     ebp, esp
    //51                             push    ecx
    //89 4D FC                       mov     [ebp+var_4], ecx
    //8B 4D FC                       mov     ecx, [ebp+var_4]
    //E8 xx xx xx xx                 call    sub_10001099
    //8B 45 08                       mov     eax, [ebp+arg_0]
    //83 E0 01                       and     eax, 1
    //85 C0                          test    eax, eax
    //74 0C                          jz      short skip
    //8B 4D FC                       mov     ecx, [ebp+var_4]
    //51                             push    ecx
    //E8 F0 56 05 00                 call    operator delete(void *)
    //83 C4 04                       add     esp, 4
    //
    //               skip:
    //8B 45 FC                       mov     eax, [ebp+var_4]
    //8B E5                          mov     esp, ebp
    //5D                             pop     ebp
    //C2 04 00                       retn    4

    t = SN_scalardtr;
    a = getRelCallTarget(x+10);
    if (gate && Byte(a)==0xE9)
    {
      //E9 xx xx xx xx   jmp   xxxxxxx
      a = getRelJmpTarget(a);
    }
  }
  else if (matchBytes(x,"568D71??578D7E??8BCFE8????????F644240C01"))
  {
    //56                             push    esi
    //8D 71 xx                       lea     esi, [ecx-XX]
    //57                             push    edi
    //8D 7E xx                       lea     edi, [esi+XX]
    //8B CF                          mov     ecx, edi
    //E8 xx xx xx xx                 call    class::~class()
    //F6 44 24 0C 01                 test    [esp+4+arg_0], 1
    a = getRelCallTarget(x+10);
    if (gate && Byte(a)==0xE9)
    {
      a = getRelJmpTarget(a);
    }
    t=SN_scalardtr;
  }
  else if (matchBytes(x,"568DB1????????578DBE????????8BCFE8????????F644240C01"))
  {
    //56                             push    esi
    //8D B1 xx xx xx xx              lea     esi, [ecx-XX]
    //57                             push    edi
    //8D BE xx xx xx xx              lea     edi, [esi+XX]
    //8B CF                          mov     ecx, edi
    //E8 xx xx xx xx                 call    class::~class()
    //F6 44 24 0C 01                 test    [esp+4+arg_0], 1
    a = getRelCallTarget(x+16);
    if (gate && Byte(a)==0xE9)
    {
      a = getRelJmpTarget(a);
    }
    t = SN_scalardtr;
  }
  else if ((matchBytes(x,"F644240401568BF1C706") /*&& Dword(x+10)==vtable*/) || 
           (matchBytes(x,"8A442404568BF1A801C706") /*&& Dword(x+11)==vtable */) ||
           (matchBytes(x,"568BF1C706????????E8????????F64424080174") && matchBytes(x+21+Byte(x+20),"8BC65EC20400"))
          )
  {
    //F6 44 24 04 01                 test    [esp+arg_0], 1
    //56                             push    esi
    //8B F1                          mov     esi, ecx
    //  OR
    //8A 44 24 04                    mov     al, [esp+arg_0]
    //56                             push    esi
    //8B F1                          mov     esi, ecx
    //A8 01                          test    al, 1

    //C7 06 xx xx xx xx              mov     dword ptr [esi], xxxxxxx //offset vtable
    //                           <inlined destructor>
    //74 07                          jz      short @@no_free
    //56                             push    esi
    //E8 CA 2D 0D 00                 call    operator delete(void *)
    //59                             pop     ecx
    //   @@no_free:
    //8B C6                          mov     eax, esi
    //5E                             pop     esi
    //C2 04 00                       retn    4  
    t = SN_scalardtr;
  }
  else if (matchBytes(x,"538A5C2408568BF1F6C302742B8B46FC578D7EFC68????????506A??56E8") || 
           matchBytes(x,"538A5C2408F6C302568BF1742E8B46FC5768????????8D7EFC5068????????56E8"))
  {
     //53                            push    ebx
     //8A 5C 24 08                   mov     bl, [esp+arg_0]
     //56                            push    esi
     //8B F1                         mov     esi, ecx
     //F6 C3 02                      test    bl, 2
     //74 2B                         jz      short loc_100037F8
     //8B 46 FC                      mov     eax, [esi-4]
     //57                            push    edi
     //8D 7E FC                      lea     edi, [esi-4]
     //68 xx xx xx xx                push    offset class::~class(void)
     //50                            push    eax
     //6A xx                         push    xxh
     //56                            push    esi
     //E8 xx xx xx xx                call    `eh vector destructor iterator'(void *,uint,int,void (*)(void *))
    t = SN_vectordtr;
    Message("  vector deleting destructor at %a\n",x);
    if (name!=0)
    a = Dword(x+21);
    if (gate && Byte(a)==0xE9)
    {
      a = getRelJmpTarget(a);
    }
  }

  if (t>0)
  {
    if (t==SN_vectordtr)
      s = "vector";
    else
      s = "scalar";
    Message("  %s deleting destructor at %a\n",s,x);
    fprintf(f, "  %s deleting destructor: %08.8Xh\n",s,x);
    if (name!=0)
      MakeName(x, MakeSpecialName(name,t,0));
    if (a!=BADADDR)
    {
      Message("  virtual destructor at %a\n",a);
      fprintf(f, "  destructor: %08.8Xh\n",a);
      if (name!=0)
        MakeName(a, MakeSpecialName(name,SN_vdestructor,0));
    }
    CommentStack(x, 4, "__flags$",-1);
  }
  return t;
}

static ParseVtbl2()
{
  auto a, n;
  a = ScreenEA();
  if (GetVtableSize(a)==0)
  {
    Warning("This location doesn't look like a vtable!");
    return;
  }
  if (!hasName(GetFlags(a)) && !IsValidCOL(Dword(a-4)))
  {
    n = AskStr("","Enter class name");
    if (n!=0)
      MakeName(a,"??_7"+n+"@@6B@");
  }
  DoVtable(a,0);
}

static AddHotkeys()
{
  AddHotkey("Alt-F7","ParseFI");
  AddHotkey("Alt-F8","ParseVtbl2");
  AddHotkey("Alt-F9","ParseExc");
  Message("Use Alt-F7 to parse FuncInfo\n");
  Message("Use Alt-F8 to parse vtable\n");
  Message("Use Alt-F9 to parse throw info\n");
}

static main(void)
{
  if(AskYN(1, "Do you wish to scan the executable for vtables/RTTI?"))
  {
    Message("Scanning...");
    scan_for_vtables();
    //Message("See objtree.txt for the class list/hierarchy.\n");
    Exec("objtree.txt");
  }
  AddHotkeys();
}