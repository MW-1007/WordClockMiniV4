#include "TimezoneMapper.h"

#include <cstring>

const char* tzPosixFromIana(const char* iana) {
  if (!iana || !*iana) return "UTC0";

  if (!strcmp(iana, "Europe/Berlin") ||
      !strcmp(iana, "Europe/Amsterdam") ||
      !strcmp(iana, "Europe/Paris") ||
      !strcmp(iana, "Europe/Rome") ||
      !strcmp(iana, "Europe/Madrid") ||
      !strcmp(iana, "Europe/Vienna") ||
      !strcmp(iana, "Europe/Zurich") ||
      !strcmp(iana, "Europe/Prague") ||
      !strcmp(iana, "Europe/Warsaw") ||
      !strcmp(iana, "Europe/Stockholm") ||
      !strcmp(iana, "Europe/Copenhagen") ||
      !strcmp(iana, "Europe/Oslo")) {
    return "CET-1CEST,M3.5.0,M10.5.0/3";
  }
  if (!strcmp(iana, "Europe/London")) return "GMT0BST,M3.5.0/1,M10.5.0";

  if (!strcmp(iana, "America/New_York")) return "EST5EDT,M3.2.0,M11.1.0";
  if (!strcmp(iana, "America/Chicago")) return "CST6CDT,M3.2.0,M11.1.0";
  if (!strcmp(iana, "America/Denver")) return "MST7MDT,M3.2.0,M11.1.0";
  if (!strcmp(iana, "America/Los_Angeles")) return "PST8PDT,M3.2.0,M11.1.0";

  if (!strcmp(iana, "Asia/Tokyo")) return "JST-9";
  if (!strcmp(iana, "Asia/Shanghai")) return "CST-8";
  if (!strcmp(iana, "Asia/Singapore")) return "SGT-8";
  if (!strcmp(iana, "Asia/Dubai")) return "GST-4";

  if (!strcmp(iana, "Australia/Sydney")) return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  if (!strcmp(iana, "UTC")) return "UTC0";

  return "UTC0";
}
