#include <idc.idc>
//Microsoft C++ RTTI support for IDA
//Version 3.0 2006.01.20 Igor Skochinsky <skochinsky@mail.ru>

//#define DEBUG

//////////////////////////////////////
// Unknown(long ea, long length)
//////////////////////////////////////
// Mark the ea as unknown for a length
// of length, but don't propagate.
static Unknown( ea, length )
{
  auto i;
  if (ea==BADADDR)
    return;
//  Message("Unknown(%x,%d)\n",ea, length);
  for(i=0; i < length; i++)
     {
       MakeUnkn(ea+i,0);
     }
}

static ForceQword( x ) {  //Make dword, undefine as needed
  if (x==BADADDR || x==0)
    return;
 if (!MakeQword( x ))
 {
   Unknown(x,8);
   MakeQword(x);
 }
}

static ForceDword( x ) {  //Make dword, undefine as needed
 if (x==BADADDR || x==0)
   return;
 if (!MakeDword( x ))
 {
   Unknown(x,4);
   MakeDword(x);
 }
}

static ForceWord( x ) {  //Make word, undefine as needed
  if (x==BADADDR || x==0)
    return;
 if (!MakeWord( x ))
 {
   Unknown(x,2);
   MakeWord( x );
 }
}

static ForceByte( x ) {  //Make byte, undefine as needed
  if (x==BADADDR || x==0)
    return;
 if (!MakeByte( x ))
 {
   MakeUnkn(x,0);
   MakeByte( x );
 }
}

static SoftOff ( x ) { //Make offset if !=0
  if (x==BADADDR || x==0)
    return;
 ForceDword(x);
 if (Dword(x)>0 && Dword(x)<=MaxEA()) OpOff(x,0,0);
}

static GetAsciizStr(x)
{
  auto s,c;
  if (x==BADADDR || x==0)
    return "";
  s = "";
  while (c=Byte(x))
  {
    s = form("%s%c",s,c);
    x = x+1;
  }
  return s;
}

//check if Dword(vtbl-4) points to typeinfo record and extract the type name from it
static GetTypeName(vtbl)
{
  auto x, s, c;
  if (vtbl==BADADDR)
    return;
  x = Dword(vtbl-4);
  if ((!x) || (x==BADADDR)) return "";
//  if (Dword(x)||Dword(x+4)||Dword(x+8)) return "";
  x = Dword(x+12);
  if ((!x) || (x==BADADDR)) return "";
  s = "";
  x = x+8;
  while (c=Byte(x))
  {
    s = form("%s%c",s,c);
    x = x+1;
  }
  return s;
}

static DwordCmt(x, cmt)
{
  if (x==BADADDR || x==0)
    return;
  ForceDword(x);
  MakeComm(x, cmt);
}
static OffCmt(x, cmt)
{
  if (x==BADADDR || x==0)
    return;
  SoftOff(x);
  MakeComm(x, cmt);
}
static StrCmt(x, cmt)
{
  auto save_str;
  if (x==BADADDR || x==0)
    return;
  MakeUnkn(x, 0);
  save_str = GetLongPrm(INF_STRTYPE);
  SetLongPrm(INF_STRTYPE,0);
  MakeStr(x, BADADDR);
  MakeName(x, "");
  MakeComm(x, cmt);
  SetLongPrm(INF_STRTYPE,save_str);
}
static DwordArrayCmt(x, n, cmt)
{
  if (x==BADADDR || x==0)
    return;
  Unknown(x,4*n);
  ForceDword(x);
  MakeArray(x,n);
  MakeComm(x, cmt);
}

//check if values match a pattern
static matchBytes(addr,match)
{
  auto i,len,s;
  len = strlen(match);
  if (len%2) 
  { 
    Warning("Bad match string in matchBytes: %s",match);
    return 0;
  }
  i=0;
  while (i<len)
  {
    s = substr(match,i,i+2);
    if (s!="??" && form("%02X",Byte(addr))!=s)
      return 0;//mismatch
    i = i+2;
    addr++;
  }
  return 1;
}

static ForceDWMember(id, offset, name)
{
  if (0!=AddStrucMember(id, name,offset, FF_DWRD, -1, 4))
    SetMemberName(id, offset, name);
}

static ForceStrucMember(id, offset, sub_id, name)
{
  auto a,i;
  i = GetStrucSize(sub_id);
  if (0!=AddStrucMember(id,name,offset,FF_DATA|FF_STRU,sub_id,i))
  {
    for (a=offset;a<offset+i;a++)
      DelStrucMember(id,a);
    AddStrucMember(id,name,offset,FF_DATA|FF_STRU,sub_id,i);
    //SetMemberName(id, offset, name);
  }
}

//add (or rename) a stack variable named name at frame offset offset (i.e. bp-based)
//struc_id = structure variable
//if struc_id == -1, then add a dword
static CommentStack(start, offset, name, struc_id)
{
  auto id,l,bp;
  id = GetFrame(start);
  l = GetFrameLvarSize(start);
  if ( (GetFunctionFlags(start) & FUNC_FRAME) == 0)
    l = l + GetFrameRegsSize(start);
  l = l+offset;
  //Message("%a: ebp offset = %02Xh\n",start,l);
  if (l<0)
  {    
    //Message("growing the frame to locals=%d, regs=4, args=%d.\n",-offset, GetFrameArgsSize(start));
    //we need to grow the locals
    MakeFrame(start, -offset, GetFrameRegsSize(start), GetFrameArgsSize(start));
    l = 0;
  }
  if (struc_id==-1)
    ForceDWMember(id, l, name);
  else
    ForceStrucMember(id, l, struc_id, name);
}

static getRelJmpTarget(a)
{
  auto b;
  b = Byte(a);
  if (b == 0xEB)
  {
    b = Byte(a+1);
    if (b&0x80)
      return a+2-((~b&0xFF)+1);
    else
      return a+2+b;
  }
  else if (b==0xE9)
  {
    b = Dword(a+1);
    if (b&0x80000000)
      return a+5-(~b+1);
    else
      return a+5+b;
  }
  else
    return BADADDR;
}

static getRelCallTarget(a)
{
  auto b;
  b = Byte(a);
  if (b==0xE8)
  {
    b = Dword(a+1);
    if (b&0x80000000)
      return a+5-(~b+1);
    else
      return a+5+b;
  }
  else
    return BADADDR;
}

static MangleNumber(x)
{
//
// 0 = A@
// X = X-1 (1<=X<=10)
// -X = ?(X-1)
// 0x0..0xF = 'A'..'P'

  auto s, sign;
  s=""; sign=0;
  if (x<0)
  {
    sign = 1;
    x = -x;
  }  
  if (x==0)
    return "A@";
  else if (x<=10)
    return form("%s%d",sign?"?":"",x-1);
  else
  {
    while (x>0)
    {
      s = form("%c%s",'A'+x%16,s);
      x = x / 16;
    }
    return sign?"?":""+s+"@";
  }
}
static Parse_BCD(x, indent)
{
  auto indent_str,i,a,s;
  if (x==BADADDR || x==0)
    return;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
/*
struct _s_RTTIBaseClassDescriptor
{
    struct TypeDescriptor* pTypeDescriptor; //type descriptor of the class
    DWORD numContainedBases; //number of nested classes following in the array
    struct PMD where;        //some displacement info
    DWORD attributes;        //usually 0, sometimes 10h
    struct _s_RTTIClassHierarchyDescriptor *pClassHierarchyDescriptor; //of this base class
};

struct PMD
{
    int mdisp;  //member displacement
    int pdisp;  //vbtable displacement
    int vdisp;  //displacement inside vbtable
};
*/
#ifdef DEBUG
  Message(indent_str+"0x%08.8X: RTTIBaseClassDescriptor\n", x);
  Message(indent_str+"    pTypeDescriptor:   %08.8Xh (%s)\n", Dword(x), GetAsciizStr(Dword(x)+8));
  Message(indent_str+"    numContainedBases: %08.8Xh\n", Dword(x+4));
  Message(indent_str+"    PMD where:         (%d,%d,%d)\n", Dword(x+8), Dword(x+12), Dword(x+16));
  Message(indent_str+"    attributes:        %08.8Xh\n", Dword(x+20));
#endif

  OffCmt(x, "pTypeDescriptor");
  DwordCmt(x+4, "numContainedBases");
  DwordArrayCmt(x+8, 3, "PMD where");
  DwordCmt(x+20, "attributes");
  OffCmt(x+24, "pClassHierarchyDescriptor");

  if(substr(Name(Dword(x+24)),0,5) != "??_R3")
  {
    // assign dummy name to prevent infinite recursion
    MakeName(Dword(x+24),"??_R3"+form("%06x",x)+"@@8");
    // a proper name will be assigned shortly
    Parse_CHD(Dword(x+24),indent-1);
  }

  s = Parse_TD(Dword(x), indent+1);
  //??_R1A@?0A@A@B@@8 = B::`RTTI Base Class Descriptor at (0,-1,0,0)'
  MakeName(x,"??_R1"+MangleNumber(Dword(x+8))+MangleNumber(Dword(x+12))+
           MangleNumber(Dword(x+16))+MangleNumber(Dword(x+20))+substr(s,4,-1)+'8');
  return s;
}

static GetClassName(p)
{
  /*auto s;
  s = GetAsciizStr(Dword(p)+8);
  Message("demangling %s\n",s);
  return DemangleTIName(s);*/
  auto s,s2;
  s = "??_7"+GetAsciizStr(Dword(p)+12)+"6B@";
  //Message("demangling %s\n",s);
  s2 = Demangle(s,8);
  if (s2!=0)
    //CObject::`vftable'
    return substr(s2,0,strlen(s2)-11);
  else
    return s;
}

static DumpNestedClass(x, indent, contained)
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
    Message("%s%s%s\n", s, indent_str, GetClassName(p));
    //fprintf(f, form("%s%s%s\n",s,indent_str,GetClassName(p)));
    n = Dword(p+4);
    if (n>0) //check numContainedBases
      DumpNestedClass(a+4, indent+1, n); //nested classes following
    a=a+4*(n+1);
    i=i+n+1;
  }
}

static Parse_CHD(x, indent)
{
  auto indent_str,i,a,n,p,s;
  if (x==BADADDR || x==0)
    return;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
  //Message(indent_str+"0x%08.8X: RTTIClassHierarchyDescriptor\n", x);
  /*
  struct _s_RTTIClassHierarchyDescriptor
  {
      DWORD signature;      //always zero?
      DWORD attributes;     //bit 0 = multiple inheritance, bit 1 = virtual inheritance
      DWORD numBaseClasses; //number of classes in pBaseClassArray
      struct _s_RTTIBaseClassArray* pBaseClassArray;
  };
  */
  a = Dword(x+4);
  if ((a&3)==1)
    p = "(MI)";
  else if ((a&3)==2)
    p = "(VI)";
  else if ((a&3)==3)
    p = "(MI VI)";
  else
    p="(SI)";

#ifdef DEBUG
  Message(indent_str+"    signature:         %08.8Xh\n", Dword(x));
  Message(indent_str+"    attributes:        %08.8Xh %s\n", a, p);
  Message(indent_str+"    numBaseClasses:    %08.8Xh\n", n);
  Message(indent_str+"    pBaseClassArray:   %08.8Xh\n", a);
#endif

  DwordCmt(x, "signature");
  DwordCmt(x+4, "attributes");
  DwordCmt(x+8, "numBaseClasses");
  OffCmt(x+12, "pBaseClassArray");

  a=Dword(x+12);
  n=Dword(x+8);
  i=0;
  DumpNestedClass(a, indent, n);
  indent=indent+1;
  while(i<=n)
  {
    p = Dword(a);
    if (i==n && p!=0)
      break;
    //Message(indent_str+"    BaseClass[%02d]:  %08.8Xh\n", i, p);
    OffCmt(a, form("BaseClass[%02d]", i));
    if (i==0)
    {
      s = Parse_BCD(p,indent);
      //??_R2A@@8 = A::`RTTI Base Class Array'
      MakeName(a,"??_R2"+substr(s,4,-1)+'8');
      //??_R3A@@8 = A::`RTTI Class Hierarchy Descriptor'
      MakeName(x,"??_R3"+substr(s,4,-1)+'8');
    }
    else
      Parse_BCD(p,indent);
    i=i+1;
    a=a+4;
  }
  return s;
}

static Parse_TD(x, indent)
{
  auto indent_str,i,a;
  if (x==BADADDR || x==0)
    return;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
  //Message(indent_str+"0x%08.8X: TypeDescriptor\n", x);
/*
struct TypeDescriptor
{
    void* pVFTable;  //always pointer to type_info::vftable ?
    void* spare;     //seems to be zero for most classes, and default constructor for exceptions
    char name[0];    //mangled name, starting with .?A (.?AV=classes, .?AU=structs)
};
*/
  a = GetAsciizStr(x+8);
#ifdef DEBUG
  Message(indent_str+"    pVFTable:          %08.8Xh\n", Dword(x));
  Message(indent_str+"    spare:             %08.8Xh\n", Dword(x+4));
  Message(indent_str+"    name:              '%s'\n", a);
#endif

  OffCmt(x, "pVFTable");
  OffCmt(x+4, "spare");
  StrCmt(x+8, "name");

  //??_R0?AVA@@@8 = A `RTTI Type Descriptor'
  MakeName(x,"??_R0"+substr(a,1,-1)+"@8");
  return a;
}


static Parse_COL(x, indent)
{
/*
struct _s_RTTICompleteObjectLocator
{
    DWORD signature; //always zero ?
    DWORD offset;    //offset of this vtable in the class ?
    DWORD cdOffset;  //no idea
    struct TypeDescriptor* pTypeDescriptor; //TypeDescriptor of the class
    struct _s_RTTIClassHierarchyDescriptor* pClassDescriptor; //inheritance hierarchy
};*/
  auto indent_str,i,a,s;
  if (x==BADADDR || x==0)
    return;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
  s = GetAsciizStr(Dword(x+12)+8);
  //Message(indent_str+"0x%08.8X: RTTICompleteObjectLocator\n", x);
#ifdef DEBUG
  Message(indent_str+"    signature:         %08.8Xh\n", Dword(x));
  Message(indent_str+"    offset:            %08.8Xh\n", Dword(x+4));
  Message(indent_str+"    cdOffset:          %08.8Xh\n", Dword(x+8));
  Message(indent_str+"    pTypeDescriptor:   %08.8Xh (%s)\n", Dword(x+12), DemangleTIName(s));
  Message(indent_str+"    pClassDescriptor:  %08.8Xh\n", Dword(x+16));
#endif

  DwordCmt(x, "signature");
  DwordCmt(x+4, "offset");
  DwordCmt(x+8, "cdOffset");
  OffCmt(x+12, "pTypeDescriptor");
  OffCmt(x+16, "pClassDescriptor");

  //
  Parse_CHD(Dword(x+16),indent+1);
}

static Parse_CT(x, indent)
{
  auto indent_str,i,a,s;
  if (x==BADADDR || x==0)
    return;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
/*
typedef const struct _s__CatchableType {
  unsigned int properties;
  _TypeDescriptor *pType;
  _PMD thisDisplacement;
  int sizeOrOffset;
  _PMFN copyFunction;
} _CatchableType;

struct PMD
{
    int mdisp;  //members displacement ???
    int pdisp;  //
    int vdisp;  //vtable displacement ???
};
*/

  s = GetAsciizStr(Dword(x+4)+8);

#ifdef DEBUG
  Message(indent_str+"0x%08.8X: CatchableType\n", x);
  Message(indent_str+"    properties:        %08.8Xh\n", Dword(x));
  Message(indent_str+"    pType:             %08.8Xh (%s)\n", Dword(x+4), DemangleTIName(s));
  Message(indent_str+"    thisDisplacement:  (%d,%d,%d)\n", Dword(x+8), Dword(x+12), Dword(x+16));
  Message(indent_str+"    sizeOrOffset:      %08.8Xh\n", Dword(x+20));
  Message(indent_str+"    copyFunction:      %08.8Xh\n", Dword(x+24));
#endif

  a = "properties";
  i = Dword(x);
  if (i!=0) a = a+":";
  if (i&1) a = a+" simple type";
  if (i&2) a = a+" byref only";
  if (i&4) a = a+" has vbases";

  DwordCmt(x, a);
  OffCmt(x+4, "pType");
  DwordArrayCmt(x+8, 3, "thisDisplacement");
  DwordCmt(x+20, "sizeOrOffset");
  OffCmt(x+24, "copyFunction");
  ForceDword(x+28);

  //__CT??_R0 ?AVCTest@@ @81 = CTest::`catchable type'
  MakeName(x,"__CT??_R0?"+substr(s,1,-1)+"@81");

  if (Dword(x+24)) //we have a copy constructor
  //.?AVexception@@ -> ??0exception@@QAE@ABV0@@Z = exception::exception(exception const &)
     MakeName(Dword(x+24),"??0"+substr(s,4,-1)+"QAE@ABV0@@Z");
  return s;
}

static Parse_CTA(x, indent)
{
/*
typedef const struct _s__CatchableTypeArray {
  int nCatchableTypes;
  _CatchableType *arrayOfCatchableTypes[];
} _CatchableTypeArray;
*/  

  auto indent_str,i,a,n,p,s;
  if (x==BADADDR || x==0)
    return;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }

#ifdef DEBUG
  Message(indent_str+"    nCatchableTypes:         %08.8Xh\n", Dword(x));
  Message(indent_str+"    arrayOfCatchableTypes:   %08.8Xh\n", Dword(x+4));
#endif
  DwordCmt(x, "nCatchableTypes");
  //OffCmt(x+4, "arrayOfCatchableTypes");

  a=x+4;
  n=Dword(x);
  i=0;
  indent=indent+1;
  while(i<n)
  {
    p = Dword(a);
    //Message(indent_str+"    BaseClass[%02d]:  %08.8Xh\n", i, p);
    OffCmt(a, form("CatchableType[%02d]", i));
    if (i==0)
    {
      s = Parse_CT(p,indent);
      //__CTA1 ?AVCTest@@ = CTest::`catchable type array'
      MakeName(x,"__CTA1?"+substr(s,1,-1));
    }
    else
      Parse_CT(p,indent);
    i=i+1;
    a=a+4;
  }
  return s;
}

//demangle names like .?AVxxx, .PAD, .H etc
static DemangleTIName(s)
{
  auto i;
  if (substr(s,0,1)!=".")
    return "";
  s = Demangle("??_R0"+substr(s,1,-1)+"@8",8);
  i = strstr(s,"`RTTI Type Descriptor'");
  if (i==-1)
    return "";
  else
  {
    s = substr(s,0,i-1);
    //Message("throw %s;\n",s);
    return s;
  }
}

static Parse_ThrowInfo(x, indent)
{
/*
typedef const struct _s__ThrowInfo {
  unsigned int attributes;
  _PMFN pmfnUnwind;
  int (__cdecl*pForwardCompat)(...);
  _CatchableTypeArray *pCatchableTypeArray;
} _ThrowInfo;
*/
  auto indent_str,i,a,s;
  if (x==BADADDR || x==0)
    return;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
#ifdef DEBUG
  Message(indent_str+"0x%08.8X: ThrowInfo\n", x);
  Message(indent_str+"    attributes:            %08.8Xh\n", Dword(x));
  Message(indent_str+"    pmfnUnwind:            %08.8Xh\n", Dword(x+4));
  Message(indent_str+"    pForwardCompat:        %08.8Xh\n", Dword(x+8));
  Message(indent_str+"    pCatchableTypeArray:   %08.8Xh\n", Dword(x+12));
#endif

  a = "attributes";
  i = Dword(x);
  if (i!=0) a = a+":";
  if (i&1) a = a+" const";
  if (i&2) a = a+" volatile";

  DwordCmt(x, a);
  OffCmt(x+4, "pmfnUnwind");
  OffCmt(x+8, "pForwardCompat");
  OffCmt(x+12, "pCatchableTypeArray");
  s = Parse_CTA(Dword(x+12), indent+1);
  if (s!="")
  {
    MakeName(x,"__TI1?"+substr(s,1,-1));
    if (Dword(x+4)) //we have a destructor
      //.?AVexception@@ -> ??1exception@@UAE@XZ = exception::~exception(void)
      MakeName(Dword(x+4),"??1"+substr(s,4,-1)+"UAE@XZ");
    i = Dword(x); //attributes
    a = DemangleTIName(s);
    if (i&1) a = "const "+a;
    if (i&2) a = "volatile "+a;
    a = "throw "+a;
    Message("%s\n",a);
    MakeRptCmt(x, a);
  }
  return s;
}

static Parse_TryBlock(x, indent)
{
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
  auto indent_str,i,a,n,p,s;
  if (x==BADADDR || x==0)
    return;
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }

#ifdef DEBUG
  Message(indent_str+"    tryLow:          %d\n", Dword(x));
  Message(indent_str+"    tryHigh:         %d\n", Dword(x+4));
  Message(indent_str+"    catchHigh:       %d\n", Dword(x+8));
  Message(indent_str+"    nCatches:        %d\n", Dword(x+12));
  Message(indent_str+"    pHandlerArray:   %08.8Xh\n", Dword(x+16));
#endif

  DwordCmt(x, "tryLow");
  DwordCmt(x+4, "tryHigh");
  DwordCmt(x+8, "catchHigh");
  DwordCmt(x+12, "nCatches");
  OffCmt(x+16, "pHandlerArray");

  a=Dword(x+16);
  n=Dword(x+12);
  if (a==BADADDR || a==0 || n==0)
    return;
  i=0;
  indent=indent+1;
  while(i<n)
  {
#ifdef DEBUG
    Message(indent_str+"        adjectives:       %08.8Xh\n", Dword(a));
    Message(indent_str+"        pType:            %08.8Xh\n", Dword(a+4));
    Message(indent_str+"        dispCatchObj:     %08.8Xh\n", Dword(a+8));
    Message(indent_str+"        addressOfHandler: %08.8Xh\n", Dword(a+12));
#endif

    DwordCmt(a, "adjectives");
    OffCmt(a+4,"pType");
    DwordCmt(a+8, "dispCatchObj");
    OffCmt(a+12,"addressOfHandler");

    p = Dword(a+4);
    if (p)
    {
      s = DemangleTIName(Parse_TD(p, indent+1));
      if (Dword(a)&8) //reference
        s = s+"&";
      if (Dword(a)&2) //volatile
        s = "volatile "+s;
      if (Dword(a)&1) //const
        s = "const "+s;
      s = s+" e";
    }
    else
      s = "...";

    p = Dword(a+12);
    if (p!=0 && p!=BADADDR)
    {
      ExtLinA(Dword(a+12),0,form("; catch (%s)",s));
      ExtLinA(Dword(a+12),1,form("; states %d..%d",Dword(x),Dword(x+4)));
      p = Dword(a+8);
      if (p)
      {
        if (p&0x80000000)
          s = form("; e = [epb-%Xh]",-p);
        else
          s = form("; e = [epb+%Xh]",p);
        ExtLinA(Dword(a+12),2,s);
      }      
    }
    i=i+1;
    a=a+16;
  }
  return s;
}

static Parse_FuncInfo(x, indent)
{
/*
typedef const struct _s_FuncInfo {
  unsigned int magicNumber:29;                      //0
  unsigned int bbtFlags:3;
  int maxState;                                     //4
  const struct _s_UnwindMapEntry * pUnwindMap;      //8
  unsigned int nTryBlocks;                          //C
  const struct _s_TryBlockMapEntry * pTryBlockMap;  //10
  unsigned int nIPMapEntries;                       //14
  void * pIPtoStateMap;                             //18
  const struct _s_ESTypeList * pESTypeList;         //1C
  int EHFlags; //present only in vc8?               //20
} FuncInfo;
typedef const struct _s_UnwindMapEntry {
  int toState;          //0
  function  * action;   //4
} UnwindMapEntry;
*/  
  auto indent_str,i,a,s,n;
  if (x==BADADDR || x==0)
    return;
  if ((Dword(x)^0x19930520)>0xF) {
    Message("Magic is not 1993052Xh!\n");
    return;
  }
  indent_str="";i=0;
  while(i<indent)
  {
    indent_str=indent_str+"    ";
    i++;
  }
#ifdef DEBUG
  Message(indent_str+"0x%08.8X: FuncInfo\n", x);
  Message(indent_str+"    magicNumber:            %08.8Xh\n", Dword(x));
  Message(indent_str+"    maxState:               %d\n", Dword(x+4));
  Message(indent_str+"    pUnwindMap:             %08.8Xh\n", Dword(x+8));
#endif
  n = Dword(x+4);
  i = 0; a = Dword(x+8);
  while (i<n)
  {
#ifdef DEBUG
    Message(indent_str+"        toState:              %d\n", Dword(a));
    Message(indent_str+"        action:               %08.8Xh\n", Dword(a+4));
#endif
    DwordCmt(a, "toState");
    OffCmt(a+4, "action");
    a = a+8;
    i = i+1;
  }
#ifdef DEBUG
  Message(indent_str+"    nTryBlocks:             %d\n", Dword(x+12));
  Message(indent_str+"    pTryBlockMap:           %08.8Xh\n", Dword(x+16));
#endif
  n = Dword(x+12);
  i = 0; a = Dword(x+16);
  while (i<n)
  {
    Parse_TryBlock(a, indent+1);
    a = a+20;
    i = i+1;
  }
#ifdef DEBUG
  Message(indent_str+"    nIPMapEntries:          %d\n", Dword(x+20));
  Message(indent_str+"    pIPtoStateMap:          %08.8Xh\n", Dword(x+24));
#endif

  DwordCmt(x, "magicNumber");
  DwordCmt(x+4, "maxState");
  OffCmt(x+8, "pUnwindMap");
  //Parse_UnwindMap(Dword(x+8), indent+1);
  DwordCmt(x+12, "nTryBlocks");
  OffCmt(x+16, "pTryBlockMap");
  DwordCmt(x+20, "nIPMapEntries");
  OffCmt(x+24, "pIPtoStateMap");
  if ((Dword(x+8)-x)>=32 || Dword(x)>0x19930520)
  {
#ifdef DEBUG
    Message(indent_str+"    pESTypeList:            %08.8Xh\n", Dword(x+28));
#endif
    OffCmt(x+28, "pESTypeList");
  }
  if ((Dword(x+8)-x)>=36 || Dword(x)>0x19930521)
  {
#ifdef DEBUG
    Message(indent_str+"    EHFlags:                %08.8Xh\n", Dword(x+32));
#endif
    OffCmt(x+32, "EHFlags");
  }
  return s;
}

//get class name for this vtable instance
static GetVtableClass(x)
{
  auto offset, n, i, s, a, p;
  offset = Dword(x+4);
  x = Dword(x+16); //Class Hierarchy Descriptor

  a=Dword(x+12); //pBaseClassArray
  n=Dword(x+8);  //numBaseClasses
  i = 0;
  s = "";
  while(i<n)
  {
    p = Dword(a);
    //Message(indent_str+"    BaseClass[%02d]:  %08.8Xh\n", i, p);
    if (Dword(p+8)==offset)
    {
      //found it
      s = GetAsciizStr(Dword(p)+8);
      return s;
    }
    i=i+1;
    a=a+4;
  }
  //didn't find matching one, let's get the first vbase
  i=0;
  a=Dword(x+12);
  while(i<n)
  {
    p = Dword(a);
    if (Dword(p+12)!=-1)
    {
      s = GetAsciizStr(Dword(p)+8);
      return s;
    }
    i=i+1;
    a=a+4;
  }
  return s;
}

static Parse_Vtable(a)
{
  auto i,s,s2;
  s=GetTypeName(a);
  if (substr(s,0,3)==".?A")
  {
#ifdef DEBUG
    Message("RTTICompleteObjectLocator:      %08.8Xh\n", Dword(a-4));
#endif
    Parse_COL(Dword(a-4),0);
    Unknown(a-4,4);
    SoftOff(a-4);
    i = Dword(a-4);  //COL
    s2 = Dword(i+4); //offset
    i = Dword(i+16); //CHD
    i = Dword(i+4);  //Attributes
    if ((i&3)==0  && s2==0)
    { //Single inheritance, so we don't need to worry about duplicate names (several vtables)
      s=substr(s,4,-1);
      MakeName(a,"??_7"+s+"6B@");
      MakeName(Dword(a-4),"??_R4"+s+"6B@");
    }
    else// if ((i&3)==1)
    { 
      //Message("Multiple inheritance\n");
      s2 = GetVtableClass(Dword(a-4));
      s2 = substr(s2,4,-1);
      s  = substr(s,4,-1);
      s = s+"6B"+s2+"@";
      MakeName(a,"??_7"+s);
      MakeName(Dword(a-4),"??_R4"+s);
    }
  }
}

static ParseVtbl()
{
  Parse_Vtable(ScreenEA());
}
static ParseExc()
{
  Parse_ThrowInfo(ScreenEA(), 0);
}

static ParseFI()
{
  Parse_FuncInfo(ScreenEA(), 0);
}

static AddHotkeys()
{
  AddHotkey("Alt-F7","ParseFI");
  AddHotkey("Alt-F8","ParseVtbl");
  AddHotkey("Alt-F9","ParseExc");
  Message("Use Alt-F7 to parse FuncInfo\n");
  Message("Use Alt-F8 to parse vtable\n");
  Message("Use Alt-F9 to parse throw info\n");
}
#ifndef __INCLUDED
static main(void)
{
  AddHotkeys();
}
#endif
