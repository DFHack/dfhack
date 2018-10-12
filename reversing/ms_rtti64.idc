#include <idc.idc>

#define isRef(F)        ((F & FF_REF)  != 0)
#define hasName(F)      ((F & FF_NAME) != 0)

static GetVtableSize(a)
{
  auto b,c,f;
  b = BADADDR;
  f = GetFlags(a);
  do {
    f = GetFlags(a);
    if (b == BADADDR) //first entry
    {
      b=a;
      if (!(isRef(f) && (hasName(f) || (f&FF_LABL))))
      {
        return 0;
      }
    }
    else if (isRef(f)) //might mean start of next vtable
      break;

    if (!hasValue(f) || !isData(f))
      break;
    c = Qword(a);
    if (c)
    {
      f = GetFlags(c);
      if (!hasValue(f) || !isCode(f) || Dword(c)==0)
        break;
    }
    a = a+8;
  }
  while (1);
  if (b!=BADADDR)
  {
    c = (a-b)/8;
    return c;
  }
  else
  {
    return 0;
  }
}

static Unknown( ea, length )
{
  auto i;
  if (ea==BADADDR)
    return;
  for(i=0; i < length; i++)
     {
       MakeUnkn(ea+i,0);
     }
}

static DwordRel(x)
{
	x = Dword(x) + 0x140000000;
	return x;
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

static SoftOff64 ( x ) { //Make 64-bit offset if !=0
  if (x==BADADDR || x==0)
    return;
 ForceQword(x);
 if (Qword(x)>0 && Qword(x)<=MaxEA()) OpOff(x, 0, 0);
}

static SoftOff ( x ) { //Make 32-bit base-relative offset if !=0
  if (x==BADADDR || x==0)
    return;
 ForceDword(x);
 if (Dword(x)>0 && Dword(x)<=MaxEA()) OpOffEx(x,0,REF_OFF32|REFINFO_RVA|REFINFO_SIGNEDOP, -1, 0, 0);
}

//check if pointer is to typeinfo record and extract the type name from it
static GetTypeName(vtbl)
{
  auto x;
  if (vtbl==BADADDR)
    return;
  x = DwordRel(vtbl+12);
  if ((!x) || (x==BADADDR)) return "";
  return GetAsciizStr(x+16);
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
  if (cmt != "")
    MakeComm(x, cmt);
}
static OffCmt64(x, cmt)
{
  if (x==BADADDR || x==0)
    return;
  SoftOff64(x);
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
  if (l<0)
  {    
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
  if (x > 2147483647) x = x - 4294967296;
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
  OffCmt(x, "pTypeDescriptor");
  DwordCmt(x+4, "numContainedBases");
  DwordArrayCmt(x+8, 3, "PMD where");
  DwordCmt(x+20, "attributes");
  OffCmt(x+24, "pClassHierarchyDescriptor");

  if(substr(Name(DwordRel(x+24)),0,5) != "??_R3")
  {
    // assign dummy name to prevent infinite recursion
    MakeName(DwordRel(x+24),"??_R3"+form("%06x",x)+"@@8");
    // a proper name will be assigned shortly
    Parse_CHD(DwordRel(x+24),indent-1);
  }

  s = Parse_TD(DwordRel(x), indent+1);
  //??_R1A@?0A@A@B@@8 = B::`RTTI Base Class Descriptor at (0,-1,0,0)'
  MakeName(x,"??_R1"+MangleNumber(Dword(x+8))+MangleNumber(Dword(x+12))+
           MangleNumber(Dword(x+16))+MangleNumber(Dword(x+20))+substr(s,4,-1)+'8');
  return s;
}

static GetClassName(p)
{
  auto s,s2;
  s = "??_7"+GetAsciizStr(DwordRel(p)+20)+"6B@";
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
    p = DwordRel(a);
    off = Dword(p+8);
    s = form("%.4X: ",off);
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

  DwordCmt(x, "signature");
  DwordCmt(x+4, "attributes");
  DwordCmt(x+8, "numBaseClasses");
  OffCmt(x+12, "pBaseClassArray");

  a=DwordRel(x+12);
  n=Dword(x+8);
  i=0;
  DumpNestedClass(a, indent, n);
  indent=indent+1;
  while(i<=n)
  {
    p = DwordRel(a);
    if (i==n && p!=0)
      break;
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
/*
struct TypeDescriptor
{
    void* pVFTable;  //always pointer to type_info::vftable ?
    void* spare;     //seems to be zero for most classes, and default constructor for exceptions
    char name[0];    //mangled name, starting with .?A (.?AV=classes, .?AU=structs)
};
*/
  a = GetAsciizStr(x+16);

  OffCmt64(x, "pVFTable");
  OffCmt64(x+8, "spare");
  StrCmt(x+16, "name");

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
  s = GetAsciizStr(DwordRel(x+12)+16);

  DwordCmt(x, "signature");
  DwordCmt(x+4, "offset");
  DwordCmt(x+8, "cdOffset");
  OffCmt(x+12, "pTypeDescriptor");
  OffCmt(x+16, "pClassDescriptor");
  OffCmt(x+20, "");

  //
  Parse_CHD(DwordRel(x+16),indent+1);
}

//get class name for this vtable instance
static GetVtableClass(x)
{
  auto offset, n, i, s, a, p;
  offset = Dword(x+4);
  x = DwordRel(x+16); //Class Hierarchy Descriptor

  a=DwordRel(x+12); //pBaseClassArray
  n=Dword(x+8);  //numBaseClasses
  i = 0;
  s = "";
  while(i<n)
  {
    p = DwordRel(a);
    if (DwordRel(p+16)==offset)
    {
      //found it
      s = GetAsciizStr(DwordRel(p)+16);
      return s;
    }
    i=i+1;
    a=a+8;
  }
  //didn't find matching one, let's get the first vbase
  i=0;
  a=DwordRel(x+12);
  while(i<n)
  {
    p = DwordRel(a);
    if (DwordRel(p+20)!=-1)
    {
      s = GetAsciizStr(DwordRel(p)+16);
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
  s=GetTypeName(Qword(a-8));
  if (substr(s,0,3)==".?A")
  {
    Parse_COL(Qword(a-8),0);
    Unknown(a-8,8);
    SoftOff64(a-8);
    i = Qword(a-8);  //COL
    s2 = Dword(i+4); //offset
    i = DwordRel(i+16); //CHD
    i = Dword(i+4);  //Attributes
    if ((i&3)==0  && s2==0)
    { //Single inheritance, so we don't need to worry about duplicate names (several vtables)
      s=substr(s,4,-1);
      MakeName(a,"??_7"+s+"6B@");
      MakeName(Qword(a-8),"??_R4"+s+"6B@");
    }
    else// if ((i&3)==1)
    { 
      s2 = GetVtableClass(Qword(a-8));
      s2 = substr(s2,4,-1);
      s  = substr(s,4,-1);
      s = s+"6B"+s2+"@";
      MakeName(a,"??_7"+s);
      MakeName(Qword(a-8),"??_R4"+s);
    }
  }
}

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

static GetVtblName(col)
{
  auto i, s, s2;
  s = GetTypeName(col);
  i = DwordRel(col+16); //CHD
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
  x = DwordRel(col+12);
  if ((!x) || (x==BADADDR)) return "";
  x = Dword(x+16);
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

static doAddrList(name)
{
  auto idx, id, val, ctr, dtr;
  id = GetArrayId("AddrList");
  ctr = 0; dtr = 0;
  if ( name!=0 && id != -1 )
  {
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
    MakeName(ctr, MakeSpecialName(name,SN_constructor,0));
  }
  DeleteArray(GetArrayId("AddrList"));
}

//check if there's a vtable at a and dump into to f
//returns position after the end of vtable
static DoVtable(a)
{
  auto x,y,s,p,q,i,name;

  //check if it looks like a vtable
  y = GetVtableSize(a);
  if (y == 0)
    return a+8;

  //check if it's named as a vtable
  name = Name(a);
  if (substr(name,0,4)!="??_7") name=0;
 
  x = Qword(a-8);  
  //otherwise try to get it from RTTI
  if (IsValidCOL(x))
  {
    Parse_Vtable(a);
    if (name==0)
      name = GetVtblName(x);
    MakeName(a, name);
  }
  if (name!=0)
  {
    name = ".?AV"+substr(name, 4, strstr(name,"@@6B")+2);
  }
  {
    DeleteArray(GetArrayId("AddrList"));
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
             AddAddr(q);
           }
           i = 1;
           q = p;
         }
       }
    }
    if (q)
    {           
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
      checkSDD(p,name,a,0);
      y--;
      x = x+8;
    }
    doAddrList(name);
  }
  return x;
}

static scan_for_vtables(void)
{
  auto rmin, rmax, cmin, cmax, s, a, x, y;
  s = FirstSeg();
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
  while (a<rmax)
  {
    x = Qword(a);
    if (x>=cmin && x<cmax) //methods should reside in .text
    {
       a = DoVtable(a);
    }
    else
      a = a + 8;
  }
}

//check for `scalar deleting destructor'
static checkSDD(x,name,vtable,gate)
{
  auto a,s,t;
  t = 0; a = BADADDR;

  if ((name!=0) && (substr(Name(x),0,3)=="??_") && (strstr(Name(x),substr(name,4,-1))==4))
    name=0; //it's already named

  if (Byte(x)==0xE9 || Byte(x)==0xEB) {
    //E9 xx xx xx xx   jmp   xxxxxxx
    return checkSDD(getRelJmpTarget(x),name,vtable,1);
  }
  else if (matchBytes(x,"83E9??E9")) {
    //thunk
    //83 E9 xx        sub     ecx, xx
    //E9 xx xx xx xx  jmp     class::`scalar deleting destructor'(uint)
    a = getRelJmpTarget(x+3);
    t = checkSDD(a,name,vtable,0);
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
    t = checkSDD(a,name,vtable,0);
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
    if (name!=0)
      MakeName(x, MakeSpecialName(name,t,0));
    if (a!=BADADDR)
    {
      if (name!=0)
        MakeName(a, MakeSpecialName(name,SN_vdestructor,0));
    }
    CommentStack(x, 4, "__flags$",-1);
  }
  return t;
}

static main(void)
{
    scan_for_vtables();
}