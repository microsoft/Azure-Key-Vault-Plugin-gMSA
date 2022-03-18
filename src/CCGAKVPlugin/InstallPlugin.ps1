#Need to run this script from the same directory as the CCGAKVPlugin binary. 

mkdir "C:\Program Files\CCGAKVPlugin"
copy CCGAKVPlugin.dll "C:\Program Files\CCGAKVPlugin\"

#Enables and disables specified privilege for the current user.
function enable-privilege {
 param(
  ## The privilege to adjust. This set is taken from
  ## http://msdn.microsoft.com/en-us/library/bb530716(VS.85).aspx
  [ValidateSet(
   "SeAssignPrimaryTokenPrivilege", "SeAuditPrivilege", "SeBackupPrivilege",
   "SeChangeNotifyPrivilege", "SeCreateGlobalPrivilege", "SeCreatePagefilePrivilege",
   "SeCreatePermanentPrivilege", "SeCreateSymbolicLinkPrivilege", "SeCreateTokenPrivilege",
   "SeDebugPrivilege", "SeEnableDelegationPrivilege", "SeImpersonatePrivilege", "SeIncreaseBasePriorityPrivilege",
   "SeIncreaseQuotaPrivilege", "SeIncreaseWorkingSetPrivilege", "SeLoadDriverPrivilege",
   "SeLockMemoryPrivilege", "SeMachineAccountPrivilege", "SeManageVolumePrivilege",
   "SeProfileSingleProcessPrivilege", "SeRelabelPrivilege", "SeRemoteShutdownPrivilege",
   "SeRestorePrivilege", "SeSecurityPrivilege", "SeShutdownPrivilege", "SeSyncAgentPrivilege",
   "SeSystemEnvironmentPrivilege", "SeSystemProfilePrivilege", "SeSystemtimePrivilege",
   "SeTakeOwnershipPrivilege", "SeTcbPrivilege", "SeTimeZonePrivilege", "SeTrustedCredManAccessPrivilege",
   "SeUndockPrivilege", "SeUnsolicitedInputPrivilege")]
  $Privilege,
  ## The process on which to adjust the privilege. Defaults to the current process.
  $ProcessId = $pid,
  ## Switch to disable the privilege, rather than enable it.
  [Switch] $Disable
 )

 ## Taken from P/Invoke.NET with minor adjustments.
 $definition = @'
 using System;
 using System.Runtime.InteropServices;
  
 public class AdjPriv
 {
  [DllImport("advapi32.dll", ExactSpelling = true, SetLastError = true)]
  internal static extern bool AdjustTokenPrivileges(IntPtr htok, bool disall,
   ref TokPriv1Luid newst, int len, IntPtr prev, IntPtr relen);
  
  [DllImport("advapi32.dll", ExactSpelling = true, SetLastError = true)]
  internal static extern bool OpenProcessToken(IntPtr h, int acc, ref IntPtr phtok);
  [DllImport("advapi32.dll", SetLastError = true)]
  internal static extern bool LookupPrivilegeValue(string host, string name, ref long pluid);
  [StructLayout(LayoutKind.Sequential, Pack = 1)]
  internal struct TokPriv1Luid
  {
   public int Count;
   public long Luid;
   public int Attr;
  }
  
  internal const int SE_PRIVILEGE_ENABLED = 0x00000002;
  internal const int SE_PRIVILEGE_DISABLED = 0x00000000;
  internal const int TOKEN_QUERY = 0x00000008;
  internal const int TOKEN_ADJUST_PRIVILEGES = 0x00000020;
  public static bool EnablePrivilege(long processHandle, string privilege, bool disable)
  {
   bool retVal;
   TokPriv1Luid tp;
   IntPtr hproc = new IntPtr(processHandle);
   IntPtr htok = IntPtr.Zero;
   retVal = OpenProcessToken(hproc, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, ref htok);
   tp.Count = 1;
   tp.Luid = 0;
   if(disable)
   {
    tp.Attr = SE_PRIVILEGE_DISABLED;
   }
   else
   {
    tp.Attr = SE_PRIVILEGE_ENABLED;
   }
   retVal = LookupPrivilegeValue(null, privilege, ref tp.Luid);
   retVal = AdjustTokenPrivileges(htok, false, ref tp, 0, IntPtr.Zero, IntPtr.Zero);
   return retVal;
  }
 }
'@

 $processHandle = (Get-Process -id $ProcessId).Handle
 $type = Add-Type $definition -PassThru
 $type[0]::EnablePrivilege($processHandle, $Privilege, $Disable)
}

New-Item -path "HKLM:\Software\CLASSES\Appid\{557110E1-88BC-4583-8281-6AAC6F708584}"
$sddlString = "01,00,04,80,44,00,00,00,54,00,00,00,00,00,00,00,14,00,00,00,02,00,30,00,02,00,00,00,00,00,14,00,0B,00,00,00,01,01,00,00,00,00,00,05,12,00,00,00,00,00,14,00,0B,00,00,00,01,01,00,00,00,00,00,05,06,00,00,00,01,02,00,00,00,00,00,05,20,00,00,00,20,02,00,00,01,02,00,00,00,00,00,05,20,00,00,00,20,02,00,00"
$hexSddl = $sddlString.Split(',') | ForEach-Object { "0x$_"}
New-ItemProperty -Path "HKLM:\Software\CLASSES\Appid\{557110E1-88BC-4583-8281-6AAC6F708584}" -Name "AccessPermission" -PropertyType Binary -Value ([byte[]]$hexSddl)
New-ItemProperty -Path "HKLM:\Software\CLASSES\Appid\{557110E1-88BC-4583-8281-6AAC6F708584}" -Name "LaunchPermission" -PropertyType Binary -Value ([byte[]]$hexSddl)
New-ItemProperty -Path "HKLM:\Software\CLASSES\Appid\{557110E1-88BC-4583-8281-6AAC6F708584}" -Name "DllSurrogate" -Value ""
New-item -path "HKLM:\SOFTWARE\CLASSES\CLSID\{CCC2A336-D7F3-4818-A213-272B7924213E}"
New-ItemProperty -path "HKLM:\SOFTWARE\CLASSES\CLSID\{CCC2A336-D7F3-4818-A213-272B7924213E}" -Name "AppID" -Value "{557110E1-88BC-4583-8281-6AAC6F708584}"
New-item -path "HKLM:\SOFTWARE\CLASSES\CLSID\{CCC2A336-D7F3-4818-A213-272B7924213E}\InprocServer32" -Value "C:\\Program Files\\CCGAKVPlugin\\CCGAKVPlugin.dll"
New-ItemProperty -path "HKLM:\SOFTWARE\CLASSES\CLSID\{CCC2A336-D7F3-4818-A213-272B7924213E}\InprocServer32" -Name "ThreadingModel" -Value "Both"

#set owner of key to current user
if(enable-privilege SeTakeOwnershipPrivilege){
	Write-Host "Enabled SeTakeOwnershipPrivilege privilege"
}
else{
	Write-Host "Enabling SeTakeOwnershipPrivilege privilege failed"
}

$key = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SYSTEM\CurrentControlSet\Control\CCG\COMClasses",[Microsoft.Win32.RegistryKeyPermissionCheck]::ReadWriteSubTree,[System.Security.AccessControl.RegistryRights]::takeownership)
$acl = $key.GetAccessControl()
$originalOwner = $acl.owner
$user = whoami 
$me = [System.Security.Principal.NTAccount]$user
$acl.SetOwner($me)
$key.SetAccessControl($acl)

#Add new access rule that gives full control for current user.
$acl = $key.GetAccessControl()
$idRef = [System.Security.Principal.NTAccount]($user)
$regRights = [System.Security.AccessControl.RegistryRights]::FullControl
$inhFlags = [System.Security.AccessControl.InheritanceFlags]::ContainerInherit
$prFlags = [System.Security.AccessControl.PropagationFlags]::None
$acType = [System.Security.AccessControl.AccessControlType]::Allow
$rule = New-Object System.Security.AccessControl.RegistryAccessRule($idRef, $regRights, $inhFlags, $prFlags, $acType)
$acl.AddAccessRule($rule)
$key.SetAccessControl($acl)

New-item -path  "HKLM:\SYSTEM\CurrentControlSet\Control\CCG\COMClasses\{CCC2A336-D7F3-4818-A213-272B7924213E}" -Value ""

#Set owner back to original owner and remove access rule for current user. 
$acl = $key.GetAccessControl()
$acl.RemoveAccessRule($rule)
$acl.SetOwner([System.Security.Principal.NTAccount]$originalowner)
if (enable-privilege SeRestorePrivilege){
	Write-Host "Enabled SeRestorePrivilege privilege"
}
else{
	Write-Host "Enabling SeRestorePrivilege privilege failed"
}
$key.SetAccessControl($acl)
$key.close()

#Disable privileges. 
if (enable-privilege SeRestorePrivilege -disable){
	Write-Host "Disabled SeRestorePrivilege privilege"
}
else{
	Write-Host "Disabling SeRestorePrivilege privilege failed"
}
 
if (enable-privilege SeTakeOwnershipPrivilege -disable){
	Write-Host "Disabled SeTakeOwnershipPrivilege privilege"	
}
else{
	Write-Host "Disabling SeTakeOwnershipPrivilege privilege failed"
}
