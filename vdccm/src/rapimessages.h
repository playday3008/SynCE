#ifndef RAPIMESSAGES
#define RAPIMESSAGES

namespace RapiMessages
{
  const unsigned char proxyEntriesPrefix[12] = { 0x12, 0x01, 0x00, 0x00,
                                                 0x0e, 0x00, 0x00, 0x00,
                                                 0x06, 0x01, 0x00, 0x00 };

  const char* proxyEntriesString =
    "<wap-provisioningdoc>"
       "<characteristic-query recursive=\"false\" type=\"CM_ProxyEntries\" >"
       "</characteristic-query>"
    "</wap-provisioningdoc>";

  const unsigned char proxyEntries2Prefix[12] = { 0xac, 0x02, 0x00, 0x00,
                                                  0x0e, 0x00, 0x00, 0x00,
                                                  0xa0, 0x02, 0x00, 0x00 };

  const char* proxyEntries2String =
    "<wap-provisioningdoc>"
      "<characteristic type=\"CM_ProxyEntries\" >"
        "<characteristic-query recursive=\"false\" type=\"HTTP-{18AD9FBD-F716-ACB6-FD8A-1965DB95B814}\" >"
        "</characteristic-query>"
        "<characteristic-query recursive=\"false\" type=\"null-corp-{18AD9FBD-F716-ACB6-FD8A-1965DB95B814}\" >"
        "</characteristic-query>"
      "</characteristic>"
    "</wap-provisioningdoc>";

  const unsigned char netEntriesPrefix[12] = { 0xd4, 0x01, 0x00, 0x00,
                                               0x0e, 0x00, 0x00, 0x00,
                                               0xc8, 0x01, 0x00, 0x00  };

  const char* netEntriesString =
    "<wap-provisioningdoc>"
      "<characteristic type=\"CM_NetEntries\" >"
        "<characteristic type=\"CurrentDTPTNetwork\" >"
          "<parm name=\"DestId\" value=\"{18AD9FBD-F716-ACB6-FD8A-1965DB95B814}\" />"
        "</characteristic>"
      "</characteristic>"
    "</wap-provisioningdoc>";


  const unsigned char syncQueryPrefix[12] = { 0xda, 0x01, 0x00, 0x00,
                                              0x0e, 0x00, 0x00, 0x00,
                                              0xce, 0x01, 0x00, 0x00 };

  const char* syncQueryString =
    "<wap-provisioningdoc>"
      "<characteristic type=\"Sync\" >"
        "<characteristic-query recursive=\"false\" type=\"Sources\" >"
        "</characteristic-query>"
        "<characteristic-query type=\"Settings\" >"
        "</characteristic-query>"
      "</characteristic>"
    "</wap-provisioningdoc>";

  const unsigned char syncSourcesQueryPrefix[12] = { 0xda, 0x01, 0x00, 0x00,
                                                     0x0e, 0x00, 0x00, 0x00,
                                                     0xce, 0x01, 0x00, 0x00 };

  const char* syncSourcesQueryString =
    "<wap-provisioningdoc>"
      "<characteristic type=\"Sync\" >"
        "<characteristic type=\"Sources\" >"
          "<characteristic-query type=\"{BA446E05-6BC1-4B9E-9169-593A88B3F302}\" >"
          "</characteristic-query>"
        "</characteristic>"
      "</characteristic>"
    "</wap-provisioningdoc>";

}
#endif
