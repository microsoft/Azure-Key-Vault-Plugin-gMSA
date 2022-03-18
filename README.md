# Overview

The Container Credential Guard Azure Key Vault Plugin (CCGAKV Plugin) retrieves group managed service account (gMSA) credentials stored in Azure Key Vault to facilitate the domain-join process. 

## Requirements 

On a domain controller, a gMSA for the container and a standard user account that is used to retrieve the gMSA password needs to be created. These credentials need to be stored in the Azure Key Vault in this format domain\user:password.

The key vault as well as the virtual machine (VM) that is being used to deploy the container need to have a managed identity assigned to them. For more information on managed identities please visit: https://docs.microsoft.com/en-us/azure/active-directory/managed-identities-azure-resources/overview

## How to deploy

Provide your credential spec file using the `--security-opt ` parameter in  `docker run `. For example: 
```
docker run --security-opt "credentialspec=file://contoso_webapp01.json" --hostname webapp01 -it mcr.microsoft.com/windows/server:ltsc2022 powershell
```

Example credspec with gMSA enabled: 
```json
{
  "CmsPlugins": [
      "ActiveDirectory"
    ],
    "DomainJoinConfig": {
        "DnsName": "testing.com",
        "DnsTreeName": "testing.com",
        "Guid": guid of domain (use Get-ADDomain),
        "MachineAccountName": "test",
        "NetBiosName": "testing",
        "Sid": sid of domain (use Get-ADDomain)
    },
   "ActiveDirectoryConfig": {
        "GroupManagedServiceAccounts": [
            {
                "Name": "test",
                "Scope": "testing.com"
            },
            {
                "Name": "test",
                "Scope": "testing"
            }
        ],
        "HostAccountConfig": {
            "PluginGUID": "{CCC2A336-D7F3-4818-A213-272B7924213E}",
            "PluginInput": "ObjectId="objectid of managed identity";SecretUri="url of secret stored in keyvault",
            "PortableCcgVersion": "1"
        }
    }
}
```

To check that gMSA is working correctly, run the following command in the container: 
```ps
# Replace contoso.com with your own domain
PS C:\> nltest /sc_verify:contoso.com

Flags: b0 HAS_IP  HAS_TIMESERV
Trusted DC Name \\dc01.contoso.com
Trusted DC Connection Status Status = 0 0x0 NERR_Success
Trust Verification Status = 0 0x0 NERR_Success
The command completed successfully
```

To verify the gMSA identity from within the container, run the following command and check the client name: 
```ps
PS C:\> klist get webapp01

Current LogonId is 0:0xaa79ef8
A ticket to krbtgt has been retrieved successfully.

Cached Tickets: (2)

#0>     Client: webapp01$ @ CONTOSO.COM
        Server: krbtgt/webapp01 @ CONTOSO.COM
        KerbTicket Encryption Type: AES-256-CTS-HMAC-SHA1-96
        Ticket Flags 0x40a10000 -> forwardable renewable pre_authent name_canonicalize
        Start Time: 3/21/2019 4:17:53 (local)
        End Time:   3/21/2019 14:17:53 (local)
        Renew Time: 3/28/2019 4:17:42 (local)
        Session Key Type: AES-256-CTS-HMAC-SHA1-96
        Cache Flags: 0
        Kdc Called: dc01.contoso.com
```

More details on deployment can be found here: https://docs.microsoft.com/en-us/virtualization/windowscontainers/manage-containers/gmsa-run-container. 


## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
