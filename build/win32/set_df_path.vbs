Option Explicit

Const BIF_returnonlyfsdirs   = &H0001

Dim wsh, objDlg, objF, fso, spoFile, args

Set fso = CreateObject("Scripting.FileSystemObject")

set args = Wscript.Arguments
if args.count > 0 Then
    Set ObjF = fso.GetFolder(args.Item(0))
else
    Set objDlg = WScript.CreateObject("Shell.Application")
    Set objF = objDlg.BrowseForFolder (&H0,"Select your DF folder", BIF_returnonlyfsdirs)
    if IsValue(objF) Then
        set ObjF = objF.self
    end if
end if

If fso.FileExists("DF_PATH.txt") Then
    fso.DeleteFile "DF_PATH.txt", True
End If

If IsValue(objF) Then
    If InStr(1, TypeName(objF), "Folder") > 0 Then
        Set spoFile = fso.CreateTextFile("DF_PATH.txt", True)
        spoFile.WriteLine(objF.Path)
    End If
End If

Function IsValue(obj)
    ' Check whether the value has been returned.
    Dim tmp
    On Error Resume Next
    tmp = " " & obj
    If Err <> 0 Then
        IsValue = False
    Else
        IsValue = True
    End If
    On Error GoTo 0
End Function
