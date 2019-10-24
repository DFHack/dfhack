// Get JSON from export-datatypes, feed it through json2xml.rb, and
// import it into Ghidra with this script. The script will prompt you
// to pick a datatype ~25 times (just because that seems to be the
// easiest way to turn a type name into a type in a script). Just spam
// enter for the default choice.
//
// Note: does not check the import matches your DF platform or
// version.
//
// @category Dwarf Fortress
// @author Jimbo Whales

import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.app.script.GhidraScript;
import ghidra.app.services.DataTypeManagerService;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.ArrayDataType;
import ghidra.program.model.data.CategoryPath;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.DataTypeManager;
import ghidra.program.model.data.EnumDataType;
import ghidra.program.model.data.FunctionDefinitionDataType;
import ghidra.program.model.data.GenericCallingConvention;
import ghidra.program.model.data.ParameterDefinitionImpl;
import ghidra.program.model.data.StructureDataType;
import ghidra.program.model.data.UnionDataType;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.SymbolTable;
import javax.xml.parsers.DocumentBuilderFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class DFHackXMLImport extends GhidraScript {
  DataTypeManagerService tms;
  DataTypeManager tm;
  SymbolTable st;
  java.util.HashMap<String, Type> idToType =
    new java.util.HashMap<String, Type>();
  Document xml;
  Node[] types;

  static Node child(Node parent, String childName) {
    var children = parent.getChildNodes();
    for (var i = 0; i < children.getLength(); i++) {
      if (children.item(i).getNodeName().equals(childName))
        return children.item(i);
    }
    throw new RuntimeException(childName + ": lost child");
  }

  // Actually gets child elements but returns them as nodes.
  static Node[] childNodes(Node parent) {
    var children = parent.getChildNodes();
    var cs = new java.util.ArrayList<Node>();
    for (var i = 0; i < children.getLength(); i++) {
      if (children.item(i).getNodeType() == Node.ELEMENT_NODE)
        cs.add(children.item(i));
    }
    return cs.toArray(new Node[]{});
  }

  static String attr(Node node, String attrName) {
    return node.getAttributes().getNamedItem(attrName).getNodeValue();
  }

  static long attrLong(Node node, String attrName) {
    return Long.valueOf(attr(node, attrName));
  }

  class Type {
    long size = 0;
    long alignment = 0;
    DataType type;

    Type(Node n) {
      if (attr(n, "complete").equals("true")) {
        size = attrLong(n, "size");
        alignment = attrLong(n, "alignment");
      }
    }

    boolean isUndefined() { return false; }

    DataTypeComponent addToStruct(StructureDataType s, long offset) {
      return s.insertAtOffset((int) offset, type, (int) size);
    }

    void label(Address address) {
      try {
        createData(address, type);
      } catch(Exception e) {
        throw new RuntimeException(e);
      }
    }
  }

  Type getSubType(Node n) {
    return getType(attr(n, "type"));
  }

  class Pointer extends Type {
    Pointer(Node n) {
      super(n);
      type = tm.getPointer(getSubType(n).type);
    }
  }

  class Array extends Type {
    Array(Node n) {
      super(n);
      var count = attrLong(n, "count");
      type = new ArrayDataType(getSubType(n).type, (int) count,
                               (int) (size / count), tm);
    }
  }

  class Named extends Type {
    String name;

    Named(Node n) {
      super(n);
      name = attr(n, "name");
    }
  }

  class Basic extends Named {
    boolean undefined = false;

    Basic(Node n) {
      super(n);
      if (name.equals("undefined")) {
        undefined = true;
        name = "void";
      }
      type = tms.getDataType(name);
    }

    @Override
    boolean isUndefined() { return undefined; }
  }

  class Fielded extends Named {
    java.util.ArrayList<Node> fields = new java.util.ArrayList<Node>();

    Fielded(Node n) {
      super(n);
      for (var f : childNodes(child(n, "fields")))
        fields.add(f);
    }
  }

  class Struct extends Fielded {
    Struct(Node n, String typeId) {
      super(n);
      var s = new StructureDataType(name, 0, tm);
      type = s;
      idToType.put(typeId, this);
      for (var f : fields) {
        var fieldType = getSubType(f);
        try {
          fieldType.addToStruct(s, (attrLong(f, "offset")))
            .setFieldName(attr(f, "name"));
        } catch(Exception e) {
          throw new RuntimeException(e);
        }
      }
    }
  }

  class Union extends Fielded {
    Union(Node n, String typeId) {
      super(n);
      var u = new UnionDataType(new CategoryPath("/"), name, tm);
      type = u;
      idToType.put(typeId, this);
      for (var f : fields) {
        var fieldType = getSubType(f);
        u.add(fieldType.type, (int) fieldType.size, attr(f, "name"), "");
      }
    }
  }

  class Enum extends Fielded {
    Enum(Node n) {
      super(n);
      var e = new EnumDataType(new CategoryPath("/"), name, (int) size, tm);
      type = e;
      for (var f : fields)
        e.add(attr(f, "name"), attrLong(f, "value"));
    }
  }

  class Thiscall extends Fielded {
    Thiscall(Node n, String typeId) {
      super(n);
      var t = new FunctionDefinitionDataType(name, tm);
      type = t;
      idToType.put(typeId, this);
      var returnType = getSubType(n);
      if (!returnType.isUndefined())
        t.setReturnType(returnType.type);
      var params = new java.util.ArrayList<ParameterDefinitionImpl>();
      for (var f : fields)
        params.add(new ParameterDefinitionImpl(attr(f, "name"),
                                               getSubType(f).type, ""));
      t.setArguments(params.toArray(new ParameterDefinitionImpl[]{}));
      t.setGenericCallingConvention(GenericCallingConvention.thiscall);
    }

    void label(Address address) {
      createFunction(address, name);
      new ApplyFunctionSignatureCmd(address, (FunctionDefinitionDataType) type,
                                    SourceType.IMPORTED, true, true)
        .applyTo(currentProgram, monitor);
    }
  }

  class Bitfield extends Fielded {
    Bitfield(Node n) {
      super(n);
      type = getSubType(n).type;
    }
    // TBD special addToStruct
  }

  Type makeType(Node n, String typeId) {
    switch (attr(n, "kind")) {
    case "pointer":
      return new Pointer(n);
    case "array":
      return new Array(n);
    case "basic":
      return new Basic(n);
    case "struct":
      return new Struct(n, typeId);
    case "union":
      return new Union(n, typeId);
    case "enum":
      return new Enum(n);
    case "thiscall":
      return new Thiscall(n, typeId);
    case "bitfield":
      return new Bitfield(n);

    default:
      throw new RuntimeException("unknown kind");
    }
  }

  Type getType(String typeId) {
    if (!idToType.containsKey(typeId)) {
      idToType.put(typeId, makeType(types[Integer.valueOf(typeId)], typeId));
    }
    return idToType.get(typeId);
  }

  void importNamespace(Node node, Namespace parent) {
    try {
      var namespace = st.createNameSpace(parent, attr(node, "name"),
                                         SourceType.IMPORTED);
      for (var l : childNodes(child(node, "labels"))) {
        monitor.checkCanceled();
        monitor.setProgress(monitor.getProgress() + 1);
        monitor.setMessage(attr(l, "name"));
        var address = toAddr(Long.toHexString(attrLong(l, "address")));
        st.createLabel(address, attr(l, "name"), namespace,
                       SourceType.IMPORTED);
        var type = getSubType(l);
        if (getMemoryBlock(address) == null) {
          printf("%s@%s outside program memory, skipped\n", attr(l, "name"),
                 address);
          continue;
        }
        clearListing(address, address.add(type.size));
        removeFunctionAt(address);
        type.label(address);
      }
      for (var n : childNodes(child(node, "namespaces")))
        importNamespace(n, namespace);
    } catch(Exception e) {
      throw new RuntimeException(e);
    }
  }

  public void run() {
    tm = currentProgram.getDataTypeManager();
    tms = state.getTool().getService(DataTypeManagerService.class);
    st = currentProgram.getSymbolTable();
    try {
      start();
      var dbf = DocumentBuilderFactory.newDefaultInstance();
      xml = dbf.newDocumentBuilder()
        .parse(askFile("Import export-datatypes XML", "Import XML"));
      types = childNodes(child(xml.getDocumentElement(), "types"));
      var totalLabels = xml.getElementsByTagName("label").getLength();
      printf("%d types in the XML. Importing %d labels in %d namespaces...\n",
             types.length, totalLabels,
             xml.getElementsByTagName("namespace").getLength());
      monitor.initialize(totalLabels);
      importNamespace(child(xml.getDocumentElement(), "namespace"),
                      currentProgram.getGlobalNamespace());
    } catch(Exception e) {
      end(false);
      throw new RuntimeException(e);
    }
    end(true);
  }

  // @Override
  // public AnalysisMode getScriptAnalysisMode() {
  //   return AnalysisMode.DISABLED;
  // }
}
