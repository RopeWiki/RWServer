#include "BetaC.h"

#define CCS "CCS"

int CCOLLECTIVE_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int CCOLLECTIVE_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int CCOLLECTIVE_Empty(CSym &sym);
int CCOLLECTIVE_DownloadBeta(const char *ubase, CSymList &symlist);
int CCOLLECTIVE_DownloadConditions(const char *ubase, CSymList &symlist);

int CNORTHWEST_DownloadSelect(DownloadFile &f, const char *ubase, const char *url, CSymList &symlist, vara &region);
int CNORTHWEST_DownloadBeta(const char *ubase, CSymList &symlist);

int CUSA_DownloadBeta(DownloadFile &f, const char *ubase, const char *lnk, const char *region, CSymList &symlist);
int CUSA_DownloadBeta(const char *ubase2, CSymList &symlist);

int ASOUTHWEST_DownloadLL(DownloadFile &f, CSym &sym, CSymList &symlist);
int ASOUTHWEST_DownloadBeta(DownloadFile &f, const char *ubase, const char *sub, const char *region, CSymList &symlist);
int ASOUTHWEST_DownloadBeta(const char *ubase, CSymList &symlist);

int DAVEPIMENTAL_DownloadBeta(const char *ubase, CSymList &symlist);

int ONROPE_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int ONROPE_DownloadBeta(const char *ubase, CSymList &symlist);

double CANYONCARTO_Stars(double stars);
int CANYONCARTO_DownloadRatings(CSymList &symlist);
int CANYONCARTO_DownloadPage(DownloadFile &f, const char *url, CSym &sym, CSymList &symlist);
int CANYONCARTO_DownloadBeta(const char *ubase, CSymList &symlist);
CString CANYONCARTO_Watermark(const char *scredit, const char *memory, int size);
int CANYONCARTO_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);

int WROCLAW_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int WROCLAW_DownloadBeta(const char *ubase, CSymList &symlist);

int COND365_DownloadBeta(const char *ubase, CSymList &symlist);

int CANDITION_DownloadBeta(const char *ubase, CSymList &symlist);

int CANYONEAST_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int CANYONEAST_DownloadBeta(const char *ubase, CSymList &symlist);

int TNTCANYONING_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int TNTCANYONING_DownloadBeta(const char *ubase, CSymList &symlist);

vars VWExtractString(const char *memory, const char *id);
double VWExtractStars(const char *memory, const char *id);
int VWCANYONING_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int VWCANYONING_DownloadBeta(const char *ubase, CSymList &symlist);

int HIKEAZ_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int HIKEAZ_GetCondition(const char *memory, CSym &sym);
int HIKEAZ_DownloadBeta(const char *ubase, CSymList &symlist);
int HIKEAZ_DownloadConditions(const char *ubase, CSymList &symlist);

int SUMMIT_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int SUMMIT_DownloadBeta(const char *ubase, CSymList &symlist);

int CLIMBUTAH_DownloadDetails(DownloadFile &f, CSym &sym, CSymList &symlist);
int CLIMBUTAH_DownloadBeta(const char *ubase, CSymList &symlist);

int CCHRONICLES_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int CCHRONICLES_DownloadBeta(const char *ubase, CSymList &symlist);

int KARSTPORTAL_DownloadPage(DownloadFile &f, const char *id, CSymList &symlist, const char *type);
int KARSTPORTAL_DownloadBeta(const char *ubase, CSymList &symlist);

int CICARUDECLAN_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int CICARUDECLAN_DownloadBeta(const char *ubase, CSymList &symlist);

int SCHLUCHT_DownloadBeta(const char *ubase, CSymList &symlist);
int SCHLUCHT_DownloadConditions(const char *ubase, CSymList &symlist);

int INFO_DownloadBeta(const char *ubase, CSymList &symlist);

int ALTISUD_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int ALTISUD_DownloadBeta(const char *ubase, CSymList &symlist);

int GUARAINFO_DownloadBeta(const char *ubase, CSymList &symlist);

int TRENCANOUS_DownloadBeta(const char *ubase, CSymList &symlist);

int ALTOPIRINEO_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int ALTOPIRINEO_DownloadBeta(const char *ubase, CSymList &symlist);

int ZIONCANYON_DownloadBeta(const char *ubase, CSymList &symlist);

int KIWICANYONS_DownloadBeta(const char *ubase, CSymList &symlist);

int CANONISMO_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int CANONISMO_DownloadBeta(const char *ubase, CSymList &symlist);

int CNEWMEXICO_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int CNEWMEXICO_DownloadBeta(const char *ubase, CSymList &symlist);

int SPHERE_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int SPHERE_DownloadBeta(const char *ubase, CSymList &symlist);

int WIKIPEDIA_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int WIKIPEDIA_DownloadBeta(const char *ubase, CSymList &symlist);

int LAOS_DownloadBetaList(const char *url, const char *dep, CSymList &symlist);
int LAOS_DownloadBeta(const char *ubase, CSymList &symlist);

int JALISCO_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int JALISCO_DownloadBeta(const char *ubase, CSymList &symlist);

int DESCENSO_DownloadBeta(const char *ubase, CSymList &symlist);

int AZORES_DownloadPage(const char *memory, CSym &sym);

int BOOKAZORES_DownloadKML(CSym &sym, vara &descents, const char *file);
int BOOKAZORES_DownloadBeta(const char *ubase, CSymList &symlist);

int BARRANQUISMO_DownloadPage(DownloadFile &fout, const char *url, CSym &sym);
int BARRANQUISMO_GetName(const char *title, vars &name, vars &aka);
int BARRANQUISMOKML_DownloadBeta(const char *ubase, CSymList &symlist);
int BARRANQUISMO_DownloadBeta(const char *ubase, CSymList &symlist, int(*DownloadPage)(DownloadFile &f, const char *url, CSym &sym));
int BARRANQUISMO_DownloadBeta(const char *ubase, CSymList &symlist);
int BARRANQUISMODB_DownloadBeta(const char *ubase, CSymList &symlist);
int BARRANCOSORG_DownloadBeta(const char *ubase, CSymList &symlist);
