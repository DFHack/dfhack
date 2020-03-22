import java.io.File;
import java.io.FileInputStream;
import java.util.*;

import javax.xml.stream.*;

import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.script.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.util.*;
import ghidra.util.task.*;

public class import_df_structures extends GhidraScript
{
	private static final String xmlnsLD = "http://github.com/peterix/dfhack/lowered-data-definition";

	@Override
	public AnalysisMode getScriptAnalysisMode()
	{
		return AnalysisMode.SUSPENDED;
	}

	private File codegenFile, symbolsFile;
	private CodeGen codegen;
	private Symbols symbols;
	private SymbolTable symbolTable;
	private DataTypeManager dtm;
	private Category dtc, dtcStd, dtcEnums, dtcVTables, dtcVMethods;
	private DataType dtUint8, dtUint16, dtUint32, dtUint64;
	private DataType dtInt8, dtInt16, dtInt32, dtInt64;
	private DataType dtInt, dtLong, dtSizeT;
	private DataType dtString, dtFStream, dtVectorBool, dtBitArray, dtDeque;
	private int baseClassPadding;

	@Override
	protected void run() throws Exception
	{
		this.codegenFile = askFile("Select codegen.out.xml", "Select");
		this.symbolsFile = askFile("Select symbols.xml", "Select");

		createStdDataTypes();
		processXMLInputs();
		this.symbolTable = symbols.findTable(currentProgram);
		println("selected symbol table: " + symbolTable.name);
		preprocessTypes();
		createDataTypes();
		labelVTables();
		labelGlobals();
	}

	private void updateProgressMajor(String message) throws Exception
	{
		monitor.checkCanceled();

		monitor.initialize(TaskMonitor.NO_PROGRESS_VALUE);
		monitor.setMessage(message);
		println(message);
	}

	private DataType createDataType(Category category, DataType dt) throws Exception
	{
		monitor.checkCanceled();

		dt = category.addDataType(dt, DataTypeConflictHandler.REPLACE_HANDLER);
		println("created data type " + category.getName() + "::" + dt.getName());
		return dt;
	}

	private DataType createDataType(Category category, String name, DataType dt) throws Exception
	{
		return createDataType(category, new TypedefDataType(name, dt));
	}
	private DataType createVectorType(DataType target) throws Exception
	{
		if (target == null)
			target = DataType.DEFAULT;
		if (BooleanDataType.dataType.isEquivalent(target))
			target = dtInt8;
		var ptr = dtm.getPointer(target, currentProgram.getDefaultPointerSize());
		var name = "vector<" + target.getName() + ">";

		var existing = dtcStd.getDataType(name);
		if (existing != null)
			return existing;

		var vec = new StructureDataType(name, 0);
		vec.setToDefaultAlignment();
		vec.add(ptr, "_M_start", null);
		vec.add(ptr, "_M_finish", null);
		vec.add(ptr, "_M_end_of_storage", null);

		return createDataType(dtcStd, vec);
	}
	private DataType createSetType(DataType target) throws Exception
	{
		if (target == null)
			target = DataType.DEFAULT;

		var name = "set<" + target.getName() + ">";
		var existing = dtcStd.getDataType(name);
		if (existing != null)
			return existing;

		Structure node = new StructureDataType("_Rb_tree_node<" + target.getName() + ">", 0);
		node.setToDefaultAlignment();
		node = (Structure)createDataType(dtcStd, node);

		var set = new StructureDataType(name, 0);
		set.setToDefaultAlignment();

		if (baseClassPadding == 1)
		{
			// GCC

			Structure nodeBase = new StructureDataType("_Rb_tree_node_base<" + target.getName() + ">", 0);
			nodeBase.setToDefaultAlignment();
			nodeBase = (Structure)createDataType(dtcStd, nodeBase);
			nodeBase.add(BooleanDataType.dataType, "_M_color", null);
			nodeBase.add(dtm.getPointer(node, currentProgram.getDefaultPointerSize()), "_M_parent", null);
			nodeBase.add(dtm.getPointer(node, currentProgram.getDefaultPointerSize()), "_M_left", null);
			nodeBase.add(dtm.getPointer(node, currentProgram.getDefaultPointerSize()), "_M_right", null);
			node.add(nodeBase, "_M_base", null);

			set.add(nodeBase, "_M_header", null);
		}
		else
		{
			// MSVC

			node.add(dtm.getPointer(node, currentProgram.getDefaultPointerSize()), "_Left", null);
			node.add(dtm.getPointer(node, currentProgram.getDefaultPointerSize()), "_Parent", null);
			node.add(dtm.getPointer(node, currentProgram.getDefaultPointerSize()), "_Right", null);
			node.add(BooleanDataType.dataType, "_Color", null);
			node.add(BooleanDataType.dataType, "_Isnil", null);

			set.add(dtm.getPointer(node, currentProgram.getDefaultPointerSize()), "_Myhead", null);
		}

		node.add(target, "_M_value_field", null);

		set.add(dtSizeT, "_M_node_count", null);

		return createDataType(dtcStd, set);
	}
	private DataType createDfArrayType(DataType target) throws Exception
	{
		if (target == null)
			target = DataType.DEFAULT;
		var ptr = dtm.getPointer(target, currentProgram.getDefaultPointerSize());
		var name = "DfArray<" + target.getName() + ">";

		var existing = dtc.getDataType(name);
		if (existing != null)
			return existing;

		var arr = new StructureDataType(name, 0);
		arr.setToDefaultAlignment();
		arr.add(ptr, "ptr", null);
		arr.add(dtInt, "length", null);

		return createDataType(dtc, arr);
	}

	private void createStdDataTypes() throws Exception
	{
		updateProgressMajor("erasing existing data types...");
		dtm = currentProgram.getDataTypeManager();
		dtm.getRootCategory().removeCategory("df", monitor);
		dtc = dtm.createCategory(new CategoryPath("/df"));

		updateProgressMajor("creating stdlib types...");
		dtcStd = dtc.createCategory("std");
		this.dtUint8 = createDataType(dtcStd, "uint8_t", AbstractIntegerDataType.getUnsignedDataType(1, dtm));
		this.dtUint16 = createDataType(dtcStd, "uint16_t", AbstractIntegerDataType.getUnsignedDataType(2, dtm));
		this.dtUint32 = createDataType(dtcStd, "uint32_t", AbstractIntegerDataType.getUnsignedDataType(4, dtm));
		this.dtUint64 = createDataType(dtcStd, "uint64_t", AbstractIntegerDataType.getUnsignedDataType(8, dtm));
		this.dtInt8 = createDataType(dtcStd, "int8_t", AbstractIntegerDataType.getSignedDataType(1, dtm));
		this.dtInt16 = createDataType(dtcStd, "int16_t", AbstractIntegerDataType.getSignedDataType(2, dtm));
		this.dtInt32 = createDataType(dtcStd, "int32_t", AbstractIntegerDataType.getSignedDataType(4, dtm));
		this.dtInt64 = createDataType(dtcStd, "int64_t", AbstractIntegerDataType.getSignedDataType(8, dtm));
		this.dtLong = createDataType(dtcStd, "long", AbstractIntegerDataType.getSignedDataType(currentProgram.getDefaultPointerSize(), dtm));
		this.dtSizeT = createDataType(dtcStd, "size_t", AbstractIntegerDataType.getUnsignedDataType(currentProgram.getDefaultPointerSize(), dtm));
		this.dtInt = createDataType(dtcStd, "int", AbstractIntegerDataType.getSignedDataType(4, dtm));

		var stringDataType = new StructureDataType("string", 0);
		var bitVecDataType = new StructureDataType("vector<bool>", 0);
		var fStreamDataType = new StructureDataType("fstream", 0);
		var dequeDataType = new StructureDataType("deque", 0);
		stringDataType.setToDefaultAlignment();
		bitVecDataType.setToDefaultAlignment();
		fStreamDataType.setToDefaultAlignment();
		dequeDataType.setToDefaultAlignment();
		switch (currentProgram.getExecutableFormat())
		{
		case "Executable and Linking Format (ELF)":
		case "Mac OS X Mach-O":
			var rep = new StructureDataType("_string_rep", 0);
			rep.setToDefaultAlignment();
			rep.add(dtSizeT, "_M_length", null);
			rep.add(dtSizeT, "_M_capacity", null);
			rep.add(dtInt, "_M_refcount", null);
			createDataType(dtcStd, rep);

			var dataPlus = new UnionDataType("_string_dataplus");
			dataPlus.setToDefaultAlignment();
			dataPlus.add(dtm.getPointer(rep, currentProgram.getDefaultPointerSize()));
			dataPlus.add(dtm.getPointer(new TerminatedStringDataType(), currentProgram.getDefaultPointerSize()));
			createDataType(dtcStd, dataPlus);

			stringDataType.add(dataPlus, "_M_p", null);

			var biterator = new StructureDataType("_bit_iterator", 0);
			biterator.setToDefaultAlignment();
			biterator.add(dtm.getPointer(dtSizeT, currentProgram.getDefaultPointerSize()), "_M_p", null);
			biterator.add(dtUint32, "_M_offset", null);
			createDataType(dtcStd, biterator);

			bitVecDataType.add(biterator, "_M_start", null);
			bitVecDataType.add(biterator, "_M_finish", null);
			bitVecDataType.add(dtm.getPointer(dtSizeT, currentProgram.getDefaultPointerSize()), "_M_end_of_storage", null);

			fStreamDataType.setMinimumAlignment(currentProgram.getDefaultPointerSize());
			fStreamDataType.add(Undefined.getUndefinedDataType(61 * currentProgram.getDefaultPointerSize() + 40));

			dequeDataType.setMinimumAlignment(currentProgram.getDefaultPointerSize());
			dequeDataType.add(Undefined.getUndefinedDataType(10 * currentProgram.getDefaultPointerSize()));

			this.baseClassPadding = 1;

			break;
		case "Portable Executable (PE)":
			var stringVal = new UnionDataType("_string_val");
			stringVal.setToDefaultAlignment();
			stringVal.add(StringDataType.dataType, 16, "_Buf", null);
			stringVal.add(dtm.getPointer(TerminatedStringDataType.dataType, currentProgram.getDefaultPointerSize()), "_Ptr", null);

			stringDataType.add(createDataType(dtcStd, stringVal), "_Bx", null);
			stringDataType.add(dtSizeT, "_Mysize", null);
			stringDataType.add(dtSizeT, "_Myres", null);

			bitVecDataType.setMinimumAlignment(currentProgram.getDefaultPointerSize());
			bitVecDataType.add(Undefined.getUndefinedDataType(4 * currentProgram.getDefaultPointerSize()));

			fStreamDataType.setMinimumAlignment(currentProgram.getDefaultPointerSize());
			fStreamDataType.add(Undefined.getUndefinedDataType(22 * currentProgram.getDefaultPointerSize() + 96));

			dequeDataType.setMinimumAlignment(currentProgram.getDefaultPointerSize());
			dequeDataType.add(Undefined.getUndefinedDataType(5 * currentProgram.getDefaultPointerSize()));

			this.baseClassPadding = currentProgram.getDefaultPointerSize();

			break;
		default:
			throw new Exception("unexpected exe format " + currentProgram.getExecutableFormat());
		}
		this.dtFStream = createDataType(dtcStd, fStreamDataType);
		this.dtString = createDataType(dtcStd, stringDataType);
		this.dtVectorBool = createDataType(dtcStd, bitVecDataType);
		this.dtDeque = createDataType(dtcStd, dequeDataType);

		var bitArrayDataType = new StructureDataType("BitArray", 0);
		bitArrayDataType.setToDefaultAlignment();
		bitArrayDataType.add(new PointerDataType(new Undefined1DataType()), "ptr", null);
		bitArrayDataType.add(AbstractIntegerDataType.getUnsignedDataType(currentProgram.getDefaultPointerSize(), dtm), "count", null);
		this.dtBitArray = createDataType(dtc, bitArrayDataType);

		this.dtcEnums = dtc.createCategory("enums");
		this.dtcVTables = dtc.createCategory("vtables");
		this.dtcVMethods = dtcVTables.createCategory("methods");
	}

	private void processXMLInputs() throws Exception
	{
		updateProgressMajor("Parsing codegen.out.xml...");
		processXMLInput(this.codegenFile);
		updateProgressMajor("Parsing symbols.xml...");
		processXMLInput(this.symbolsFile);
	}

	private interface IHasName
	{
		void setName(String name);
	}
	private interface IHasValue
	{
		void setValue(long value);
	}
	private interface IHasStringValue
	{
		void setValue(String value);
	}
	private interface ILoweredData
	{
		void setMeta(String meta);
		void setSubtype(String subtype);
	}
	private interface IHasAnonName
	{
		void setAnonName(String name);
	}
	private interface IHasTypeName
	{
		void setTypeName(String name);
		void setBaseType(String name);
	}
	private interface IOwnsType
	{
		TypeDef getOwnedType();
	}
	private interface IHasFields
	{
		List<TypeDef.Field> getFields();
	}

	private static abstract class NameHaver implements IHasName
	{
		public boolean hasName;
		public String name;

		@Override
		public void setName(String name)
		{
			this.hasName = true;
			this.name = name;
		}
	}
	private static abstract class NameValueHaver extends NameHaver implements IHasValue
	{
		public boolean hasValue;
		public long value;

		@Override
		public void setValue(long value)
		{
			this.hasValue = true;
			this.value = value;
		}
	}
	private static abstract class AnonNameHaver extends NameHaver implements IHasAnonName
	{
		public boolean hasAnonName;
		public String anonName;

		@Override
		public void setAnonName(String name)
		{
			this.hasAnonName = true;
			this.anonName = name;
		}
	}

	private static class CodeGen
	{
		public final Map<String, TypeDef> typesByName = new HashMap<>();
		public final List<TypeDef> types = new ArrayList<>();
		public final List<TypeDef.Field> globals = new ArrayList<>();
	}
	private static class TypeDef implements ILoweredData, IOwnsType, IHasFields
	{
		public static class EnumItem extends NameValueHaver
		{
		}
		public static class Field extends AnonNameHaver implements ILoweredData, IOwnsType, IHasTypeName
		{
			public String typeName;
			public String baseType;
			public TypeDef ownedType;
			public String meta = "";
			public String subtype = "";
			public int size;
			public boolean hasCount;
			public int count;
			public Field item;
			public String indexEnum;
			public boolean forceEnumSize;

			@Override
			public void setMeta(String meta)
			{
				this.meta = meta;
			}
			@Override
			public void setSubtype(String subtype)
			{
				this.subtype = subtype;
			}
			@Override
			public void setTypeName(String name)
			{
				this.typeName = name;
			}
			@Override
			public void setBaseType(String name)
			{
				this.baseType = name;
			}
			@Override
			public TypeDef getOwnedType()
			{
				if (this.ownedType == null)
					this.ownedType = new TypeDef();
				return this.ownedType;
			}
		}
		public static class VMethod extends AnonNameHaver implements IHasFields
		{
			public final List<Field> arguments = new ArrayList<>();
			public Field returnType;
			public boolean isDestructor;

			@Override
			public List<Field> getFields()
			{
				return arguments;
			}
		}

		public String typeName;
		public String originalName;
		public String inheritsFrom;
		public String baseType;
		public String meta = "";
		public String subtype = "";
		public boolean isUnion;
		public boolean hasSubClasses;
		public final List<Field> fields = new ArrayList<>();
		public final List<EnumItem> enumItems = new ArrayList<>();
		public final List<VMethod> vmethods = new ArrayList<>();

		@Override
		public void setMeta(String meta)
		{
			this.meta = meta;
		}
		@Override
		public void setSubtype(String subtype)
		{
			this.subtype = subtype;
		}
		@Override
		public TypeDef getOwnedType()
		{
			return this;
		}
		@Override
		public List<Field> getFields()
		{
			return fields;
		}
	}

	private static class Symbols
	{
		public final List<SymbolTable> tables = new ArrayList<>();

		public SymbolTable findTable(Program currentProgram) throws Exception
		{
			long actualTS = 0;
			if (currentProgram.getExecutableFormat().equals("Portable Executable (PE)"))
			{
				// TODO: is there a *good* way to do this with Ghida APIs?
				var dosHeader = currentProgram.getListing().getDataAt(currentProgram.getImageBase());
				var dosHeaderType = (Structure)dosHeader.getBaseDataType();
				DataTypeComponent ntHeaderOffsetField = null;
				for (var dosHeaderField : dosHeaderType.getComponents())
				{
					if (dosHeaderField.getFieldName().equals("e_lfanew"))
					{
						ntHeaderOffsetField = dosHeaderField;
						break;
					}
				}
				var ntHeaderOffset = dosHeader.getUnsignedInt(ntHeaderOffsetField.getOffset());
				var ntHeaderAddr = currentProgram.getImageBase().add(ntHeaderOffset);
				var ntHeader = currentProgram.getListing().getDataAt(ntHeaderAddr);
				var ntHeaderType = (Structure)ntHeader.getBaseDataType();
				for (var ntHeaderField : ntHeaderType.getComponents())
				{
					if (ntHeaderField.getFieldName().equals("FileHeader"))
					{
						var fileHeader = ntHeader.getComponent(ntHeaderField.getOrdinal());
						var fileHeaderType = (Structure)fileHeader.getDataType();
						for (var fileHeaderField : fileHeaderType.getComponents())
						{
							if (fileHeaderField.getFieldName().equals("TimeDateStamp"))
							{
								actualTS = fileHeader.getUnsignedInt(fileHeaderField.getOffset());
								break;
							}
						}

						break;
					}
				}
			}
			var actualMD5 = currentProgram.getExecutableMD5();
			if (actualMD5 == null)
			{
				actualMD5 = "";
			}

			for (var table : tables)
			{
				if (table.hasBinaryTimestamp)
				{
					if (table.binaryTimestamp != actualTS)
						continue;
				}
				if (table.hasMD5Hash)
				{
					if (!table.md5Hash.equalsIgnoreCase(actualMD5))
						continue;
				}
				return table;
			}
			throw new Exception("could not find a relevant symbol table for the current program. is df-structures up to date?");
		}
	}
	private static class SymbolTable extends NameHaver
	{
		public static class VTableAddress extends NameValueHaver
		{
			public boolean hasMangledName;
			public String mangledName;
			public boolean hasOffset;
			public long offset;
		}
		public static class GlobalAddress extends NameValueHaver
		{
		}
		public class BinaryTimestamp implements IHasValue
		{
			@Override
			public void setValue(long value)
			{
				hasBinaryTimestamp = true;
				binaryTimestamp = value;
			}
		}
		public class MD5Hash implements IHasStringValue
		{
			@Override
			public void setValue(String value)
			{
				hasMD5Hash = true;
				md5Hash = value;
			}
		}

		public boolean hasBinaryTimestamp;
		public long binaryTimestamp;
		public boolean hasMD5Hash;
		public String md5Hash;
		public boolean hasOSType;
		public String osType;
		public final List<VTableAddress> vtables = new ArrayList<>();
		public final List<GlobalAddress> globals = new ArrayList<>();

		public BinaryTimestamp newBinaryTimestamp()
		{
			return new BinaryTimestamp();
		}
		public MD5Hash newMD5Hash()
		{
			return new MD5Hash();
		}
	}

	private void processXMLInput(File file) throws Exception
	{
		var factory = XMLInputFactory.newDefaultFactory();
		var inputStream = new FileInputStream(file);
		var reader = factory.createXMLStreamReader(inputStream);

		var stack = new Stack<>();

		while (reader.hasNext())
		{
			int tag = reader.next();
			switch (tag)
			{
			case XMLStreamConstants.START_ELEMENT:
				// shared variable namespace
				SymbolTable st;
				SymbolTable.VTableAddress vta;
				TypeDef.VMethod vm;

				if (reader.getNamespaceURI() == null)
				{
					switch (reader.getLocalName())
					{
					case "enum-item":
						var ei = new TypeDef.EnumItem();
						((IOwnsType)stack.peek()).getOwnedType().enumItems.add(ei);
						stack.push(ei);
						break;
					case "virtual-methods":
						stack.push(stack.peek());
						break;
					case "vmethod":
						vm = new TypeDef.VMethod();
						((IOwnsType)stack.peek()).getOwnedType().vmethods.add(vm);
						stack.push(vm);
						break;
					case "ret-type":
						vm = (TypeDef.VMethod)stack.peek();
						vm.returnType = new TypeDef.Field();
						stack.push(vm.returnType);
						break;
					case "comment":
						// ignore (for now)
						stack.push(null);
						break;
					case "data-definition":
						this.symbols = new Symbols();
						stack.push(this.symbols);
						break;
					case "symbol-table":
						st = new SymbolTable();
						this.symbols.tables.add(st);
						stack.push(st);
						break;
					case "binary-timestamp":
						st = (SymbolTable)stack.peek();
						stack.push(st.newBinaryTimestamp());
						break;
					case "md5-hash":
						st = (SymbolTable)stack.peek();
						stack.push(st.newMD5Hash());
						break;
					case "global-address":
						st = (SymbolTable)stack.peek();
						var ga = new SymbolTable.GlobalAddress();
						st.globals.add(ga);
						stack.push(ga);
						break;
					case "vtable-address":
						st = (SymbolTable)stack.peek();
						vta = new SymbolTable.VTableAddress();
						st.vtables.add(vta);
						stack.push(vta);
						break;
					default:
						printerr("Unhandled XML element name: " + reader.getLocalName());
						// fallthrough
					case "enum-attr":
					case "item-attr":
					case "code-helper":
					case "extra-include":
					case "custom-methods":
					case "cmethod":
						// ignore
						stack.push(null);
						continue;
					}
				}
				else if (reader.getNamespaceURI().equals(xmlnsLD))
				{
					switch (reader.getLocalName())
					{
					case "data-definition":
						this.codegen = new CodeGen();
						stack.push(this.codegen);
						break;
					case "global-type":
						var gtype = new TypeDef();
						((CodeGen)stack.peek()).types.add(gtype);
						stack.push(gtype);
						break;
					case "global-object":
						var gobj = new TypeDef.Field();
						((CodeGen)stack.peek()).globals.add(gobj);
						stack.push(gobj);
						break;
					case "field":
						var field = new TypeDef.Field();
						if (stack.peek() instanceof IHasFields)
							((IHasFields)stack.peek()).getFields().add(field);
						else
							((IOwnsType)stack.peek()).getOwnedType().fields.add(field);
						stack.push(field);
						break;
					case "item":
						var item = new TypeDef.Field();
						((TypeDef.Field)stack.peek()).item = item;
						stack.push(item);
						break;
					default:
						printerr("Unhandled XML element name: ld:" + reader.getLocalName());
						stack.push(null);
						continue;
					}
				}
				else
				{
					printerr("Unhandled XML element namespace: " + reader.getNamespaceURI());
					stack.push(null);
					continue;
				}

				for (int i = 0; i < reader.getAttributeCount(); i++)
				{
					if (!reader.isAttributeSpecified(i))
						continue;
					if (reader.getAttributeNamespace(i) == null)
					{
						switch (reader.getAttributeLocalName(i))
						{
						case "type-name":
							if (stack.peek() instanceof IHasTypeName)
								((IHasTypeName)stack.peek()).setTypeName(reader.getAttributeValue(i));
							else
								((IOwnsType)stack.peek()).getOwnedType().typeName = reader.getAttributeValue(i);
							break;
						case "base-type":
							if (stack.peek() instanceof IHasTypeName)
								((IHasTypeName)stack.peek()).setBaseType(reader.getAttributeValue(i));
							else
								((IOwnsType)stack.peek()).getOwnedType().baseType = reader.getAttributeValue(i);
							break;
						case "last-value":
							// ignore
							break;
						case "name":
							((IHasName)stack.peek()).setName(reader.getAttributeValue(i));
							break;
						case "value":
							if (stack.peek() instanceof IHasStringValue)
								((IHasStringValue)stack.peek()).setValue(reader.getAttributeValue(i));
							else
								((IHasValue)stack.peek()).setValue(Long.decode(reader.getAttributeValue(i)));
							break;
						case "ref-target":
							// ignore
							break;
						case "pointer-type":
							// ignore
							break;
						case "comment":
							// ignore (for now)
							break;
						case "init-value":
							// ignore
							break;
						case "count":
							((TypeDef.Field)stack.peek()).hasCount = true;
							((TypeDef.Field)stack.peek()).count = Integer.decode(reader.getAttributeValue(i));
							break;
						case "aux-value":
							// ignore
							break;
						case "since":
							// ignore
							break;
						case "refers-to":
							// ignore
							break;
						case "ret-type":
							// ignore (this becomes an element)
							break;
						case "is-destructor":
							((TypeDef.VMethod)stack.peek()).isDestructor = true;
							break;
						case "inherits-from":
							((IOwnsType)stack.peek()).getOwnedType().inheritsFrom = reader.getAttributeValue(i);
							break;
						case "index-enum":
							((TypeDef.Field)stack.peek()).indexEnum = reader.getAttributeValue(i);
							break;
						case "instance-vector":
							// ignore
							break;
						case "key-field":
							// ignore
							break;
						case "original-name":
							((IOwnsType)stack.peek()).getOwnedType().originalName = reader.getAttributeValue(i);
							break;
						case "is-union":
							((IOwnsType)stack.peek()).getOwnedType().isUnion = Boolean.parseBoolean(reader.getAttributeValue(i));
							break;
						case "is-array":
							// ignore
							break;
						case "is-list":
							// ignore
							break;
						case "default-value":
							// ignore
							break;
						case "use-key-name":
							// ignore
							break;
						case "index-refers-to":
							// ignore
							break;
						case "size":
							((TypeDef.Field)stack.peek()).size = Integer.decode(reader.getAttributeValue(i));
							break;
						case "has-bad-pointers":
							// ignore
							break;
						case "custom-methods":
							// ignore
							break;
						case "filename":
							// ignore
							break;
						case "item-type":
							// ignore (becomes an element)
							break;
						case "df-list-link-type":
							// ignore
							break;
						case "df-list-link-field":
							// ignore
							break;
						case "os-type":
							st = (SymbolTable)stack.peek();
							st.hasOSType = true;
							st.osType = reader.getAttributeValue(i);
							break;
						case "offset":
							vta = (SymbolTable.VTableAddress)stack.peek();
							vta.hasOffset = true;
							vta.offset = Long.decode(reader.getAttributeValue(i));
							break;
						case "mangled":
							vta = (SymbolTable.VTableAddress)stack.peek();
							vta.hasMangledName = true;
							vta.mangledName = reader.getAttributeValue(i);
							break;
						default:
							printerr("Unhandled XML attribute name: " + reader.getAttributeLocalName(i));
							continue;
						}
					}
					else if (reader.getAttributeNamespace(i).equals(xmlnsLD))
					{
						switch (reader.getAttributeLocalName(i))
						{
						case "meta":
							((ILoweredData)stack.peek()).setMeta(reader.getAttributeValue(i));
							break;
						case "level":
							// ignore
							break;
						case "subtype":
							((ILoweredData)stack.peek()).setSubtype(reader.getAttributeValue(i));
							break;
						case "typedef-name":
							((IOwnsType)stack.peek()).getOwnedType().typeName = reader.getAttributeValue(i);
							break;
						case "is-container":
							// ignore
							break;
						case "bits":
							// ignore
							break;
						case "unsigned":
							// ignore
							break;
						case "anon-name":
							((IHasAnonName)stack.peek()).setAnonName(reader.getAttributeValue(i));
							break;
						case "enum-size-forced":
							((TypeDef.Field)stack.peek()).forceEnumSize = true;
							break;
						case "in-union":
							// ignore
							break;
						case "anon-compound":
							((IOwnsType)stack.peek()).getOwnedType().typeName = "(anon compound)";
							break;
						default:
							printerr("Unhandled XML attribute name: ld:" + reader.getAttributeLocalName(i));
							continue;
						}
					}
					else
					{
						printerr("Unhandled XML attribute namespace: " + reader.getAttributeNamespace(i));
						continue;
					}
				}
				break;
			case XMLStreamConstants.END_ELEMENT:
				stack.pop();
				break;
			case XMLStreamConstants.CHARACTERS:
				// ignore (for now)
				break;
			case XMLStreamConstants.COMMENT:
				// ignore
				break;
			case XMLStreamConstants.END_DOCUMENT:
				// ignore
				break;
			case XMLStreamConstants.CDATA:
				// ignore
				break;
			default:
				throw new Exception("Unhandled XML type: " + tag);
			}
		}
	}

	private void preprocessTypes() throws Exception
	{
		var toAdd = new ArrayList<TypeDef>();
		for (var gobj : codegen.globals)
		{
			findAnonymousTypes(toAdd, "", gobj);
		}
		for (var gtype : codegen.types)
		{
			findAnonymousTypes(toAdd, gtype);
		}
		codegen.types.addAll(toAdd);

		for (var t : codegen.types)
		{
			codegen.typesByName.put(t.typeName, t);
			if (t.originalName != null)
				codegen.typesByName.put(t.originalName, t);
		}

		boolean foundAny = true;
		while (foundAny)
		{
			foundAny = false;

			for (var t : codegen.types)
			{
				if (!"class-type".equals(t.meta))
					continue;

				var parent = codegen.typesByName.get(t.inheritsFrom);
				if (parent != null && !parent.hasSubClasses)
				{
					parent.hasSubClasses = true;
					foundAny = true;
				}
			}
		}
	}

	private void createDataTypes() throws Exception
	{
		updateProgressMajor("Creating data types...");
		monitor.initialize(codegen.types.size());
		int i = 0;
		for (var t : codegen.types)
		{
			monitor.checkCanceled();
			createDataType(t);
			i++;
			monitor.setProgress(i);
		}
	}

	private void findAnonymousTypes(List<TypeDef> toAdd, TypeDef parent) throws Exception
	{
		var prefix = parent.typeName + "::";
		for (var field : parent.fields)
		{
			findAnonymousTypes(toAdd, prefix, field);
		}
	}

	private void findAnonymousTypes(List<TypeDef> toAdd, String prefix, TypeDef.Field field) throws Exception
	{
		for (var f = field; f != null; f = f.item)
		{
			if (f.ownedType != null)
			{
				if (f.ownedType.typeName == null)
					throw new Exception("unnamed typed field " + prefix + f.name);
				if (f.meta.equals("compound"))
				{
					if (f.subtype.isEmpty())
						f.ownedType.meta = "struct-type";
					else
						f.ownedType.meta = f.subtype + "-type";
				}
				else if (f.meta.equals("static-array"))
				{
					f.ownedType.meta = f.meta;
					var af = new TypeDef.Field();
					af.meta = f.meta;
					af.indexEnum = f.indexEnum;
					af.hasCount = f.hasCount;
					af.count = f.count;
					af.item = f.item;
					f.ownedType.fields.add(af);
				}
				f.ownedType.typeName = prefix + f.ownedType.typeName;
				f.typeName = f.ownedType.typeName;
				toAdd.add(f.ownedType);
				findAnonymousTypes(toAdd, f.ownedType);
			}
		}
	}

	private DataType createDataType(TypeDef t) throws Exception
	{
		DataType existing;
		if (t.meta.equals("enum-type"))
			existing = dtcEnums.getDataType(t.typeName);
		else
			existing = dtc.getDataType(t.typeName);
		if (existing != null)
			return existing;

		switch (t.meta)
		{
		case "enum-type":
			return createEnumDataType(t);
		case "bitfield-type":
			return createBitfieldDataType(t);
		case "struct-type":
			return createStructDataType(t);
		case "class-type":
			return createClassDataType(t);
		case "static-array":
			return createDataType(dtc, t.typeName, getDataType(t.fields.get(0)));
		default:
			throw new Exception("Unhandled type meta for " + t.typeName + ": " + t.meta);
		}
	}

	private DataType getDataType(String name) throws Exception
	{
		if (name == null)
			return null;

		return createDataType(codegen.typesByName.get(name));
	}

	private DataType getDataType(TypeDef.Field f) throws Exception
	{
		switch (f.meta)
		{
		case "primitive":
			switch (f.subtype)
			{
			case "stl-string":
				return dtString;
			case "stl-fstream":
				return dtFStream;
			}
			break;
		case "container":
			switch (f.subtype)
			{
			case "stl-vector":
				return createVectorType(f.item == null ? null : getDataType(f.item));
			case "stl-bit-vector":
				return dtVectorBool;
			case "stl-set":
				return createSetType(f.item == null ? null : getDataType(f.item));
			case "stl-deque":
				return dtcStd.addDataType(new TypedefDataType("deque<" + (f.item == null ? DataType.DEFAULT : getDataType(f.item)).getName() + ">", dtDeque), DataTypeConflictHandler.REPLACE_HANDLER);
			case "df-flagarray":
				return dtBitArray;
			case "df-array":
				return createDfArrayType(f.item == null ? null : getDataType(f.item));
			case "df-linked-list":
				return getDataType(f.typeName);
			}
			break;
		case "number":
			switch (f.subtype)
			{
			case "bool":
				return BooleanDataType.dataType;
			case "s-float":
				return Float4DataType.dataType;
			case "d-float":
				return Float8DataType.dataType;
			case "int8_t":
				return dtInt8;
			case "int16_t":
				return dtInt16;
			case "int32_t":
				return dtInt32;
			case "int64_t":
				return dtInt64;
			case "uint8_t":
				return dtUint8;
			case "uint16_t":
				return dtUint16;
			case "uint32_t":
				return dtUint32;
			case "uint64_t":
				return dtUint64;
			case "long":
				return dtLong;
			}
			break;
		case "pointer":
			if (f.item == null)
				return dtm.getPointer(DataType.DEFAULT, currentProgram.getDefaultPointerSize());

			if ("global".equals(f.item.meta) || "compound".equals(f.item.meta))
			{
				/*
				var t = codegen.typesByName.get(f.item.typeName);
				if (t != null && t.hasSubClasses)
					return dtm.getPointer(findOrCreateBaseClassUnion(t), currentProgram.getDefaultPointerSize());
				*/
			}

			return dtm.getPointer(getDataType(f.item), currentProgram.getDefaultPointerSize());
		case "global":
		case "compound":
			if (f.forceEnumSize)
			{
				return dtcStd.getDataType(f.baseType);
			}
			return getDataType(f.typeName);
		case "static-array":
			if (f.hasCount)
				return new ArrayDataType(getDataType(f.item), f.count, 0);
			var enumItems = codegen.typesByName.get(f.indexEnum).enumItems;
			return new ArrayDataType(getDataType(f.item), enumItems.size() + (int)enumItems.get(0).value, 0);
		case "bytes":
			switch (f.subtype)
			{
			case "padding":
				return new ArrayDataType(Undefined1DataType.dataType, f.size, 1);
			case "static-string":
				return StringDataType.dataType;
			}
			break;
		}
		throw new Exception("Unhandled field meta/subtype: " + f.meta + "/" + f.subtype);
	}

	private DataType createEnumDataType(TypeDef t) throws Exception
	{
		var et = new EnumDataType(t.typeName, t.baseType == null || t.baseType.isEmpty() ? 4 : dtcStd.getDataType(t.baseType).getLength());

		long prevValue = -1;
		for (var ei : t.enumItems)
		{
			long value = ei.hasValue ? ei.value : prevValue + 1;
			String name = ei.hasName ? ei.name : "_unk_" + value;
			et.add(name, value);
			prevValue = value;
		}

		return createDataType(dtcEnums, et);
	}

	private void addStructField(Composite st, TypeDef t, TypeDef.Field f) throws Exception
	{
		String name = null;
		if (f.hasName)
			name = f.name;
		else if (f.hasAnonName)
			name = t.typeName + "_" + f.anonName;

		st.add(getDataType(f), f.size, name, null);
	}

	private void addStructFields(Composite st, TypeDef t) throws Exception
	{
		if (t.inheritsFrom != null)
		{
			addStructFields(st, codegen.typesByName.get(t.inheritsFrom));

			int pastAlignment = st.getLength() % this.baseClassPadding;
			if (pastAlignment != 0)
			{
				st.add(new ArrayDataType(Undefined1DataType.dataType, this.baseClassPadding - pastAlignment, 1), null, "base class padding for " + t.typeName);
			}
		}

		for (var f : t.fields)
		{
			addStructField(st, t, f);
		}
	}

	private DataType createStructDataType(TypeDef t) throws Exception
	{
		Composite st = t.isUnion ? new UnionDataType(t.typeName) : new StructureDataType(t.typeName, 0);
		// add early to avoid recursion
		st = (Composite)dtc.addDataType(st, DataTypeConflictHandler.REPLACE_HANDLER);
		st.setToDefaultAlignment();

		addStructFields(st, t);

		if (t.originalName != null)
			throw new Exception("original name");

		return createDataType(dtc, st);
	}

	private DataType createMethodDataType(String name, TypeDef.VMethod vm) throws Exception
	{
		var ft = new FunctionDefinitionDataType(name);
		ft.setGenericCallingConvention(GenericCallingConvention.thiscall);

		if (vm.returnType != null)
			ft.setReturnType(getDataType(vm.returnType));
		else if (!vm.hasAnonName)
			ft.setReturnType(DataType.VOID);

		var args = new ParameterDefinition[vm.arguments.size()];
		for (int i = 0; i < vm.arguments.size(); i++)
		{
			var arg = vm.arguments.get(i);
			String aname = null;
			if (arg.hasName)
				aname = arg.name;
			else if (arg.hasAnonName)
				aname = arg.anonName;
			args[i] = new ParameterDefinitionImpl(aname, getDataType(arg), null);
		}
		ft.setArguments(args);

		return createDataType(dtcVMethods, ft);
	}

	private DataType createVTableDataType(TypeDef t) throws Exception
	{
		var name = t.originalName != null ? t.originalName : t.typeName;
		var existing = dtcVTables.getDataType("vtable_" + name);
		if (existing != null)
			return existing;

		Structure st = new StructureDataType("vtable_" + name, 0);
		// add early to avoid recursion
		st = (Structure)dtcVTables.addDataType(st, DataTypeConflictHandler.REPLACE_HANDLER);
		st.setToDefaultAlignment();

		if (t.inheritsFrom != null)
		{
			st.add(createVTableDataType(codegen.typesByName.get(t.inheritsFrom)), "_super", null);
		}

		for (var vm : t.vmethods)
		{
			String mname = null;
			if (vm.isDestructor)
			{
				mname = "~" + name;
				if (baseClassPadding == 1)
				{
					// GCC
					var mt = dtm.getPointer(createMethodDataType(name + "::" + mname, vm), currentProgram.getDefaultPointerSize());
					st.add(mt, mname, null);
					st.add(mt, mname + "(deleting)", null);
				}
				else
				{
					// MSVC
					if (vm.arguments.isEmpty())
					{
						var arg = new TypeDef.Field();
						arg.meta = "number";
						arg.subtype = "bool";
						arg.hasName = true;
						arg.name = "deleting";
						vm.arguments.add(arg);
					}
					var mt = dtm.getPointer(createMethodDataType(name + "::" + mname, vm), currentProgram.getDefaultPointerSize());
					st.add(mt, mname, null);
				}
				continue;
			}
			
			if (vm.hasName)
				mname = vm.name;
			else if (vm.hasAnonName)
				mname = name + "_" + vm.anonName;
			st.add(dtm.getPointer(createMethodDataType(name + "::" + mname, vm), currentProgram.getDefaultPointerSize()), mname, null);
		}

		return createDataType(dtcVTables, st);
	}

	private Union findOrCreateBaseClassUnion(TypeDef t) throws Exception
	{
		var typeName = "virtual_" + (t.originalName == null ? t.typeName : t.originalName);
		var existing = (Union)dtc.getDataType(typeName);
		if (existing != null)
			return existing;

		var ut = new UnionDataType(typeName);
		dtc.addDataType(ut, DataTypeConflictHandler.REPLACE_HANDLER);
		ut.add(createDataType(t), t.typeName, null);
		return (Union)createDataType(dtc, ut);
	}

	private void addToBaseClassUnion(TypeDef t, Structure st) throws Exception
	{
		if (t.inheritsFrom == null)
			return;

		var self = t.hasSubClasses ? findOrCreateBaseClassUnion(t) : st;
		var parent = findOrCreateBaseClassUnion(codegen.typesByName.get(t.inheritsFrom));
		parent.add(self);
	}

	private DataType createClassDataType(TypeDef t) throws Exception
	{
		Structure st = new StructureDataType(t.originalName != null ? t.originalName : t.typeName, 0);
		// add early to avoid recursion
		st = (Structure)dtc.addDataType(st, DataTypeConflictHandler.REPLACE_HANDLER);
		if (t.originalName != null)
			dtc.addDataType(new TypedefDataType(t.typeName, st), DataTypeConflictHandler.REPLACE_HANDLER);
		st.setToDefaultAlignment();
		st.add(dtm.getPointer(createVTableDataType(t), currentProgram.getDefaultPointerSize()), "_vtable", null);

		addStructFields(st, t);

		st = (Structure)createDataType(dtc, st);

		addToBaseClassUnion(t, st);

		if (t.originalName != null)
			return createDataType(dtc, t.typeName, st);
		return st;
	}

	private DataType createBitfieldDataType(TypeDef t) throws Exception
	{
		var st = new StructureDataType(t.typeName, 0);
		st.setToDefaultAlignment();
		st.setMinimumAlignment(4);

		for (var f : t.fields)
		{
			String name = null;
			if (f.hasName)
				name = f.name;
			else if (f.hasAnonName)
				name = t.typeName + "_" + f.anonName;
			int count = f.hasCount ? f.count : 1;
			st.addBitField(dtUint32, count, name, null);
		}

		return createDataType(dtc, st);
	}

	private void labelData(Address addr, DataType dt, String name, int size) throws Exception
	{
		println("labelling " + addr + " as " + name + " (" + dt.getCategoryPath().getName() + "::" + dt.getName() + ") ");
		var listing = currentProgram.getListing();
		var existing = listing.getData(new AddressSet(new AddressRangeImpl(addr, dt.getLength() == -1 ? size : dt.getLength())), true);
		for (var e : existing)
		{
			if (!e.isDefined() || Undefined.isUndefined(e.getDataType()))
			{
				listing.clearCodeUnits(e.getMinAddress(), e.getMaxAddress(), false, monitor);
			}
			else
			{
				var st = currentProgram.getSymbolTable();
				var syms = st.getSymbols(e.getAddress());
				printerr("overlapping " + e.getDataType().getName() + " " + (syms.length > 0 ? syms[0].getName() : "(unnamed)"));
			}
		}

		try
		{
			listing.createData(addr, dt, size);
		}
		catch (CodeUnitInsertionException ex)
		{
			printerr(ex.getMessage());
		}
		createLabel(addr, name, true, SourceType.IMPORTED);
	}

	private void labelVMethods(Address addr, GhidraClass cls, Structure st) throws Exception
	{
		var symtab = currentProgram.getSymbolTable();

		for (var field : st.getComponents())
		{
			if ("_super".equals(field.getFieldName()))
			{
				labelVMethods(addr, cls, (Structure)field.getDataType());
				continue;
			}

			Address fnaddr;
			if (currentProgram.getDefaultPointerSize() == 4)
			{
				fnaddr = toAddr(currentProgram.getMemory().getInt(addr.add(field.getOffset())));
			}
			else
			{
				fnaddr = toAddr(currentProgram.getMemory().getLong(addr.add(field.getOffset())));
			}

			var funcType = (FunctionDefinition)((Pointer)field.getDataType()).getDataType();
			var cmd = new CreateFunctionCmd(field.getFieldName(), fnaddr, null, SourceType.IMPORTED);
			Function func;
			if (cmd.applyTo(currentProgram))
			{
				func = cmd.getFunction();
			}
			else
			{
				func = currentProgram.getListing().getFunctionAt(fnaddr);
				if (func != null && !func.getSignatureSource().isLowerPriorityThan(SourceType.IMPORTED))
				{
					func = null;
				}
			}
			if (func != null)
			{
				func.setName(field.getFieldName(), SourceType.IMPORTED);
				func.setParentNamespace(cls);
				var ret = new ReturnParameterImpl(funcType.getReturnType(), currentProgram);
				var params = new ArrayList<Variable>();
				for (var arg : funcType.getArguments())
					params.add(new ParameterImpl(arg.getName(), arg.getDataType(), currentProgram));

				func.updateFunction("__thiscall", ret, params, Function.FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS, true, SourceType.IMPORTED);
			}
			else
			{
				symtab.createLabel(fnaddr, field.getFieldName(), cls, SourceType.IMPORTED);
			}
		}
	}

	private void labelVTable(Namespace ns, Address addr, GhidraClass cls, DataType dt) throws Exception
	{
		labelData(addr, dt, dt.getName(), 0);
		labelVMethods(addr, cls, (Structure)dt);
	}

	private void labelVTables() throws Exception
	{
		updateProgressMajor("Labelling vtables...");
		monitor.initialize(symbolTable.vtables.size());

		var symtab = currentProgram.getSymbolTable();
		var ns = symtab.getNamespace("df", null);
		if (ns == null)
			ns = symtab.createNameSpace(null, "df", SourceType.IMPORTED);

		int i = 0;
		for (var vt : symbolTable.vtables)
		{
			monitor.setProgress(i++);

			if (!vt.hasName)
				continue;

			var dt = dtcVTables.getDataType("vtable_" + vt.name);
			if (dt == null)
				continue;

			var cls = symtab.createClass(ns, vt.name, SourceType.IMPORTED);

			long offset = vt.hasOffset ? vt.offset : 0;

			if (vt.hasValue)
			{
				labelVTable(ns, toAddr(vt.value + offset), cls, dt);
			}

			if (vt.hasMangledName)
			{
				var syms = symtab.getGlobalSymbols(vt.mangledName);
				if (syms.isEmpty())
					continue;

				for (var s : syms)
				{
					labelVTable(ns, s.getAddress().add(offset), cls, dt);
				}
			}
		}
	}

	private void labelGlobals() throws Exception
	{
		updateProgressMajor("Labelling globals...");
		monitor.initialize(codegen.globals.size());

		var addrs = new HashMap<String, Address>();

		for (var g : symbolTable.globals)
		{
			if (!g.hasName)
				continue;
			if (!g.hasValue)
				continue;

			addrs.put(g.name, toAddr(g.value));
		}

		int i = 0;
		for (var gobj : codegen.globals)
		{
			monitor.setProgress(i++);

			if (!gobj.hasName)
				continue;
			if (!addrs.containsKey(gobj.name))
				continue;

			var dt = getDataType(gobj.item);
			if (dt == null)
				throw new Exception("missing data type for global " + gobj.name);

			labelData(addrs.get(gobj.name), dt, gobj.name, gobj.item.size);
		}
	}
}
