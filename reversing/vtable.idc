#include <idc.idc>
static GetVtableSize(a)
{
  auto b,c,f;
  b = BADADDR;
  f = GetFlags(a);
  //Message("checking vtable at: %a\n",a);
  do {
    f = GetFlags(a);
    if (b == BADADDR) //first entry
    {
      b=a;
      if (!(isRef(f) && (hasName(f) || (f&FF_LABL))))
      {
        //Message("Start of vtable should have a xref and a name (auto or manual)\n");
        return 0;
      }
    }
    else if (isRef(f)) //might mean start of next vtable
      break;

    //Message("hasValue(f):%d, isData(f):%d, isOff0(f):%d, (f & DT_TYPE) != FF_DWRD:%d\n",
    //  hasValue(f), isData(f), isOff0(f), (f & DT_TYPE) != FF_DWRD);
    if (!hasValue(f) || !isData(f) /*|| !isOff0(f) || (f & DT_TYPE) != FF_DWRD*/)
      break;
    c = Dword(a);
    if (c)
    {
      f = GetFlags(c);
      if (!hasValue(f) || !isCode(f) || Dword(c)==0)
        break;
    }
    a = a+4;
  }
  while (1);
  if (b!=BADADDR)
  {
    c = (a-b)/4;
    //Message("vtable: %08X-%08X, methods: %d\n",b,a,c);
    return c;
  }
  else
  {
    //Message("no vtable at this EA\n");
    return 0;
  }
}

static main(void)
{
  auto a, c, k, name, i, struct_id, bNameMethods,e, methName;
  a = ScreenEA();
  k = GetVtableSize(a);
  if (k>100)
  {
    if (1!=AskYN(0,form("%08X: This vtable appears to have %d methods. Are you sure you want to continue?",a,k)))
      return;
  }
  if (hasName(GetFlags(a)))
    name = Name(a);
  else
    name = "";
  if (substr(name,0,4)=="??_7")
    name = substr(name,4,strlen(name)-5);
  name = AskStr(name,"Please enter the class name");
  if (name==0)
    return;
  struct_id = GetStrucIdByName(name+"_vtable");
  if (struct_id != -1)
  {
    i = AskYN(0,form("A vtable structure for %s already exists. Are you sure you want to remake it?",name));
    if (i==-1)
      return;
    if (i==1)
    {
      DelStruc(struct_id);
      struct_id = AddStrucEx(-1,name+"_vtable",0);
    }
  }
  else
    struct_id = AddStrucEx(-1,name+"_vtable",0);
  if (struct_id == -1)
    Warning("Could not create the vtable structure!.\nPlease check the entered class name.");
  bNameMethods = (1==AskYN(0,form("Would you like to assign auto names to the virtual methods (%s_virtXX)?",name)));
  for (i=0;i<k;i++)
  {
    c = Dword(a+i*4);
    if (bNameMethods && !hasName(GetFlags(c)))
        MakeName(c,form("%s_virt%02X",name,i*4));
    if (hasName(GetFlags(c)))
    {
      methName = Name(c);
      if (substr(methName,0,1)=="?")
      {
        methName = substr(methName,1,strstr(methName,"@"));
      }
    }
    else
      methName = form("virt%02X",i*4);
    e = AddStrucMember(struct_id,methName,i*4,FF_DWRD|FF_DATA,-1,4);
    if (0!=e)
    {
      if (e!=-2 && e!=-1)
      {
        Warning(form("Error adding a vtable entry (%d)!",e));
        return;
      }
      else if (substr(GetMemberName(struct_id, i*4),0,6)=="field_")
        SetMemberName(struct_id, i*4, form("virt%02X",i*4));
    }
    SetMemberComment(struct_id,i*4,form("-> %08X, args: 0x%X",c,GetFrameArgsSize(c)),1);
  }
  MakeName(a,"??_7"+name+"@@6B@");
}
