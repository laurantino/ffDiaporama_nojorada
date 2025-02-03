/* ======================================================================
    This file is part of ffDiaporama
    ffDiaporama is a tool to make diaporama as video
    Copyright (C) 2011-2014 Dominique Levray <domledom@laposte.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
   ====================================================================== */

// Specific inclusions
#include "_Variables.h"
#include "cLocation.h"

cHTMLConversion *_HTMLConverter = NULL;
cVariable       Variable;

cHTMLConversion *HTMLConverter()
{
   if(_HTMLConverter == NULL )
      _HTMLConverter = new cHTMLConversion();
   return _HTMLConverter;
}
//=========================================================================================================================================

cHTMLConversion::cHTMLConversion() {
    //List.append(cHTMLConversionItem("'","&#39;"));
    List.append(cHTMLConversionItem("&","&amp;"));
    List.append(cHTMLConversionItem("–","&ndash;"));    List.append(cHTMLConversionItem("—","&mdash;"));
    List.append(cHTMLConversionItem("¡","&iexcl;"));    List.append(cHTMLConversionItem("¿","&iquest;"));
    List.append(cHTMLConversionItem("\"","&quot;"));    List.append(cHTMLConversionItem("÷","&divide;"));
    List.append(cHTMLConversionItem("“","&ldquo;"));    List.append(cHTMLConversionItem("”","&rdquo;"));
    List.append(cHTMLConversionItem("‘","&lsquo;"));    List.append(cHTMLConversionItem("’","&rsquo;"));
    List.append(cHTMLConversionItem("«","&laquo;"));    List.append(cHTMLConversionItem("»","&raquo;"));
    List.append(cHTMLConversionItem("©","&copy;"));     List.append(cHTMLConversionItem("™","&trade;"));
    List.append(cHTMLConversionItem(">","&gt;"));       List.append(cHTMLConversionItem("<","&lt;"));
    List.append(cHTMLConversionItem("µ","&micro;"));    List.append(cHTMLConversionItem("·","&middot;"));
    List.append(cHTMLConversionItem("¶","&para;"));     List.append(cHTMLConversionItem("±","&plusmn;"));
    List.append(cHTMLConversionItem("€","&euro;"));     List.append(cHTMLConversionItem("£","&pound;"));
    List.append(cHTMLConversionItem("¢","&cent;"));     List.append(cHTMLConversionItem("¥","&yen;"));
    List.append(cHTMLConversionItem("®","&reg;"));      List.append(cHTMLConversionItem("§","&sect;"));
    List.append(cHTMLConversionItem("á","&aacute;"));   List.append(cHTMLConversionItem("Á","&Aacute;"));
    List.append(cHTMLConversionItem("à","&agrave;"));   List.append(cHTMLConversionItem("À","&Agrave;"));
    List.append(cHTMLConversionItem("â","&acirc;"));    List.append(cHTMLConversionItem("Â","&Acirc;"));
    List.append(cHTMLConversionItem("å","&aring;"));    List.append(cHTMLConversionItem("Å","&Aring;"));
    List.append(cHTMLConversionItem("ã","&atilde;"));   List.append(cHTMLConversionItem("Ã","&Atilde;"));
    List.append(cHTMLConversionItem("ä","&auml;"));     List.append(cHTMLConversionItem("Ä","&Auml;"));
    List.append(cHTMLConversionItem("æ","&aelig;"));    List.append(cHTMLConversionItem("Æ","&AElig;"));
    List.append(cHTMLConversionItem("ç","&ccedil;"));   List.append(cHTMLConversionItem("Ç","&Ccedil;"));
    List.append(cHTMLConversionItem("é","&eacute;"));   List.append(cHTMLConversionItem("É","&Eacute;"));
    List.append(cHTMLConversionItem("è","&egrave;"));   List.append(cHTMLConversionItem("È","&Egrave;"));
    List.append(cHTMLConversionItem("ê","&ecirc;"));    List.append(cHTMLConversionItem("Ê","&Ecirc;"));
    List.append(cHTMLConversionItem("ë","&euml;"));     List.append(cHTMLConversionItem("Ë","&Euml;"));
    List.append(cHTMLConversionItem("í","&iacute;"));   List.append(cHTMLConversionItem("Í","&Iacute;"));
    List.append(cHTMLConversionItem("ì","&igrave;"));   List.append(cHTMLConversionItem("Ì","&Igrave;"));
    List.append(cHTMLConversionItem("î","&icirc;"));    List.append(cHTMLConversionItem("Î","&Icirc;"));
    List.append(cHTMLConversionItem("ï","&iuml;"));     List.append(cHTMLConversionItem("Ï","&Iuml;"));
    List.append(cHTMLConversionItem("ñ","&ntilde;"));   List.append(cHTMLConversionItem("Ñ","&Ntilde;"));
    List.append(cHTMLConversionItem("ó","&oacute;"));   List.append(cHTMLConversionItem("Ó","&Oacute;"));
    List.append(cHTMLConversionItem("ò","&ograve;"));   List.append(cHTMLConversionItem("Ò","&Ograve;"));
    List.append(cHTMLConversionItem("ô","&ocirc;"));    List.append(cHTMLConversionItem("Ô","&Ocirc;"));
    List.append(cHTMLConversionItem("ø","&oslash;"));   List.append(cHTMLConversionItem("Ø","&Oslash;"));
    List.append(cHTMLConversionItem("õ","&otilde;"));   List.append(cHTMLConversionItem("Õ","&Otilde;"));
    List.append(cHTMLConversionItem("ö","&ouml;"));     List.append(cHTMLConversionItem("Ö","&Ouml;"));
    List.append(cHTMLConversionItem("ú","&uacute;"));   List.append(cHTMLConversionItem("Ú","&Uacute;"));
    List.append(cHTMLConversionItem("ù","&ugrave;"));   List.append(cHTMLConversionItem("Ù","&Ugrave;"));
    List.append(cHTMLConversionItem("û","&ucirc;"));    List.append(cHTMLConversionItem("Û","&Ucirc;"));
    List.append(cHTMLConversionItem("ü","&uuml;"));     List.append(cHTMLConversionItem("Ü","&Uuml;"));
    List.append(cHTMLConversionItem("ß","&szlig;"));    List.append(cHTMLConversionItem("ÿ","&yuml;"));
    List.append(cHTMLConversionItem("´","&#180;"));     List.append(cHTMLConversionItem("`","&#96;"));
    //List.append(cHTMLConversionItem(" ","&nbsp;"));
    List.append(cHTMLConversionItem("\n","<br />"));
    List.append(cHTMLConversionItem("\t","&nbsp;&nbsp;&nbsp;&nbsp;"));
}
cHTMLConversion::~cHTMLConversion()
{
   List.clear();
}
// ************************************************

QString cHTMLConversion::ToHTML(QString Source) {
    for (int i=0;i<List.count();i++)
        if (Source.contains(List[i].Normal,Qt::CaseSensitive))
            Source=Source.replace(List[i].Normal,List[i].HTML,Qt::CaseSensitive);
    return Source;
}

// ************************************************

QString cHTMLConversion::ToPlainText(QString Source) {
    for (int i=0;i<List.count();i++)
        if (Source.contains(List[i].HTML,Qt::CaseSensitive))
            Source=Source.replace(List[i].HTML,List[i].Normal,Qt::CaseSensitive);
    return Source;
}

//=========================================================================================================================================

cVariableItem::cVariableItem(QString VarName) {
    this->VarName=VarName;
    Value="";
}

// ************************************************

cVariable::cVariable() {
    Variables.append(cVariableItem("STP"));
    Variables.append(cVariableItem("STM"));
    Variables.append(cVariableItem("STA"));
    Variables.append(cVariableItem("CPY"));
    Variables.append(cVariableItem("END"));
    Variables.append(cVariableItem("PTT"));
    Variables.append(cVariableItem("PAU"));
    Variables.append(cVariableItem("PAI"));
    Variables.append(cVariableItem("PAP"));
    Variables.append(cVariableItem("PAL"));
    Variables.append(cVariableItem("PLN"));
    Variables.append(cVariableItem("PLA"));
    Variables.append(cVariableItem("PCT"));
    Variables.append(cVariableItem("PDD"));
    Variables.append(cVariableItem("PSC"));
    Variables.append(cVariableItem("PLD"));
    Variables.append(cVariableItem("PSD"));
    Variables.append(cVariableItem("PYR"));
    Variables.append(cVariableItem("PMM"));
    Variables.append(cVariableItem("PMD"));
    Variables.append(cVariableItem("PDY"));
    Variables.append(cVariableItem("PDW"));
    Variables.append(cVariableItem("CSN"));
    Variables.append(cVariableItem("CSR"));
    Variables.append(cVariableItem("CCN"));
    Variables.append(cVariableItem("CCR"));
    Variables.append(cVariableItem("CCI"));
    Variables.append(cVariableItem("CCD"));
    Variables.append(cVariableItem("CCT"));
    Variables.append(cVariableItem("CLN"));
    Variables.append(cVariableItem("CLA"));
    Variables.append(cVariableItem("CLD"));
    Variables.append(cVariableItem("CSD"));
    Variables.append(cVariableItem("CYR"));
    Variables.append(cVariableItem("CMM"));
    Variables.append(cVariableItem("CMD"));
    Variables.append(cVariableItem("CDY"));
    Variables.append(cVariableItem("CDW"));
    Variables.append(cVariableItem("FFD"));
    Variables.append(cVariableItem("TLD"));
    Variables.append(cVariableItem("TSD"));
    Variables.append(cVariableItem("TYR"));
    Variables.append(cVariableItem("TMM"));
    Variables.append(cVariableItem("TMD"));
    Variables.append(cVariableItem("TDY"));
    Variables.append(cVariableItem("TDW"));
}

// ************************************************
// Text variables like %Txxx%
// ************************************************

QString cVariable::ResolveTextVariable(cDiaporamaObject *Object,QString SourceText) 
{
   if (!Object) 
      return SourceText;
#ifdef Q_OS_WIN
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
   SourceText = SourceText.replace("font-family:'Sans Serif'", "font-family:'MS Shell Dlg 2'");
   SourceText = SourceText.replace("font-family:'Serif'", "font-family:'Arial'");
#endif
#endif
   if( !SourceText.contains("%") )
      return SourceText;

   // VarName transformation
   cDiaporama          *pDiaporama = Object->pDiaporama;
   cDiaporamaObject    *CurrentChapter;
   int                 ChapterNumber = Object->ComputeChapterNumber(&CurrentChapter);
   QString             ChapterNum = QString("%1").arg(ChapterNumber);

   while (ChapterNum.length() < 3) 
      ChapterNum = "0" + ChapterNum;
   ChapterNum = "Chapter_" + ChapterNum + ":";

   QLocale loc;
   QString VarName;
   for (int i = 0; i < Variables.count(); i++) 
      if (SourceText.contains("%"+Variables[i].VarName+"%")) 
      {
         // Project properties
         if      (Variables[i].VarName == "PTT") VarName = pDiaporama->ProjectInfo()->Title();
         else if (Variables[i].VarName == "PAU") VarName = pDiaporama->ProjectInfo()->Author();
         else if (Variables[i].VarName == "PAI") VarName = QApplication::translate("Variables","%1 presents").arg(pDiaporama->ProjectInfo()->Author());
         else if (Variables[i].VarName == "PAP") VarName = QApplication::translate("Variables","A %1 production").arg(pDiaporama->ProjectInfo()->Author());
         else if (Variables[i].VarName == "PLN") 
            VarName = pDiaporama->ProjectInfo()->Location() ? pDiaporama->ProjectInfo()->Location()->Name : QApplication::translate("Variables","Project's location not set (Name)");
         else if (Variables[i].VarName == "PLA") 
            VarName = pDiaporama->ProjectInfo()->Location() ? pDiaporama->ProjectInfo()->Location()->FriendlyAddress : QApplication::translate("Variables","Project's location not set (Address)");
         else if (Variables[i].VarName == "PCT") VarName = pDiaporama->ProjectInfo()->Comment();
         else if (Variables[i].VarName == "PDD") { VarName=pDiaporama->ProjectInfo()->GetRealDuration().toString("HH:mm:ss.zzz"); VarName=VarName.left(VarName.lastIndexOf(".")); }
         else if (Variables[i].VarName == "PSC") VarName = QString("%1").arg(pDiaporama->slides.count());
                                       
         // Project date values           
         else if (Variables[i].VarName == "PLD") VarName = pDiaporama->ProjectInfo()->LongDate();
         else if (Variables[i].VarName == "PSD") VarName = pDiaporama->ProjectInfo()->EventDate().toString(pDiaporama->ApplicationConfig()->ShortDateFormat);
         else if (Variables[i].VarName == "PYR") VarName = QString("%1").arg(pDiaporama->ProjectInfo()->EventDate().year());
         else if (Variables[i].VarName == "PMD") VarName = ito2a(pDiaporama->ProjectInfo()->EventDate().month());
         else if (Variables[i].VarName == "PDY") VarName = ito2a(pDiaporama->ProjectInfo()->EventDate().day());
         //else if (Variables[i].VarName == "PMM") VarName = UpInitials(pDiaporama->ProjectInfo()->EventDate().longMonthName(pDiaporama->ProjectInfo()->EventDate().month()));
         //else if (Variables[i].VarName == "PDW") VarName = UpInitials(pDiaporama->ProjectInfo()->EventDate().longDayName(pDiaporama->ProjectInfo()->EventDate().dayOfWeek()));
         else if (Variables[i].VarName == "PMM") VarName = loc.monthName(pDiaporama->ProjectInfo()->EventDate().month());
         else if (Variables[i].VarName == "PDW") VarName = loc.dayName(pDiaporama->ProjectInfo()->EventDate().dayOfWeek());

         // Current slide                 
         else if (Variables[i].VarName == "CSN") VarName = Object->GetDisplayName();
         else if (Variables[i].VarName == "CSR") VarName = QString("%1").arg(Object->GetSlideNumber()+1);
                                                        
         // Current chapter                                
         else if (Variables[i].VarName == "CCN") VarName = GetInformationValue(ChapterNum+"title",pDiaporama->ProjectInfo()->chapterProps());
         else if (Variables[i].VarName == "CCR") VarName = QString("%1").arg(ChapterNumber);
         else if (Variables[i].VarName == "CCI") VarName = QApplication::translate("Variables","Chapter %1").arg(ChapterNumber);
         else if (Variables[i].VarName == "CCD") VarName = GetInformationValue(ChapterNum+"Duration",pDiaporama->ProjectInfo()->chapterProps());
         else if (Variables[i].VarName == "CCT") VarName = QString("%1").arg(pDiaporama->ProjectInfo()->numChapters());
         else if (Variables[i].VarName == "CLN") VarName = GetInformationValue(ChapterNum+"LocationName",pDiaporama->ProjectInfo()->chapterProps());
         else if (Variables[i].VarName == "CLA") VarName = GetInformationValue(ChapterNum+"LocationAddress",pDiaporama->ProjectInfo()->chapterProps());
                                                      
         // Current chapter date values                  
         else if (Variables[i].VarName == "CSD") VarName = GetInformationValue(ChapterNum+"Date",pDiaporama->ProjectInfo()->chapterProps());
         else if (Variables[i].VarName == "CLD") VarName = GetInformationValue(ChapterNum+"LongDate",pDiaporama->ProjectInfo()->chapterProps());
         else if (Variables[i].VarName == "CYR") VarName = QString("%1").arg((CurrentChapter->OverrideProjectEventDate ? CurrentChapter->ChapterEventDate : pDiaporama->ProjectInfo()->EventDate()).year());
         else if (Variables[i].VarName == "CMD") VarName = ito2a((CurrentChapter->OverrideProjectEventDate ? CurrentChapter->ChapterEventDate : pDiaporama->ProjectInfo()->EventDate()).month());
         else if (Variables[i].VarName == "CDY") VarName = ito2a((CurrentChapter->OverrideProjectEventDate ? CurrentChapter->ChapterEventDate : pDiaporama->ProjectInfo()->EventDate()).day());
         //else if (Variables[i].VarName == "CMM") VarName = UpInitials(QDate().longMonthName((CurrentChapter->OverrideProjectEventDate ? CurrentChapter->ChapterEventDate : pDiaporama->ProjectInfo()->EventDate()).month()));
         //else if (Variables[i].VarName == "CDW") VarName = UpInitials(QDate().longDayName((CurrentChapter->OverrideProjectEventDate ? CurrentChapter->ChapterEventDate : pDiaporama->ProjectInfo()->EventDate()).dayOfWeek()));
         else if (Variables[i].VarName == "CMM") VarName = loc.monthName((CurrentChapter->OverrideProjectEventDate ? CurrentChapter->ChapterEventDate : pDiaporama->ProjectInfo()->EventDate()).month());
         else if (Variables[i].VarName == "CDW") VarName = loc.dayName((CurrentChapter->OverrideProjectEventDate ? CurrentChapter->ChapterEventDate : pDiaporama->ProjectInfo()->EventDate()).dayOfWeek());

         // Various values                
         else if (Variables[i].VarName == "FFD") VarName = QString("%1 (%2)").arg(pDiaporama->ProjectInfo()->Composer()).arg(pDiaporama->ProjectInfo()->Revision());
         else if (Variables[i].VarName == "STP") VarName = Variables[i].Value;
         else if (Variables[i].VarName == "STM") VarName = Variables[i].Value;
         else if (Variables[i].VarName == "STA") VarName = QApplication::translate("Variables","Project done the %1:\n\t·with %2 (%3)\n\t·on a %4 (%5 Core/CPU) computer")
            .arg(QDate::currentDate().toString(pDiaporama->ApplicationConfig()->ShortDateFormat))
            .arg(pDiaporama->ProjectInfo()->Composer()).arg(pDiaporama->ProjectInfo()->Revision())
            .arg(pDiaporama->ApplicationConfig()->Plateforme).arg(getCpuCount());
         else if (Variables[i].VarName == "CPY") VarName = QString("©%1 - %2").arg(QDate::currentDate().year()).arg(pDiaporama->ProjectInfo()->Author());
         else if (Variables[i].VarName == "END") VarName = QApplication::translate("Variables","The end");

         // Today date values
         //else if (Variables[i].VarName == "TLD") VarName = UpInitials(QDate::currentDate().toString(Qt::DefaultLocaleLongDate));
         else if (Variables[i].VarName == "TLD") VarName = UpInitials(loc.toString(QDate::currentDate(), QLocale::LongFormat));
         else if (Variables[i].VarName == "TSD") VarName = QDate::currentDate().toString(pDiaporama->ApplicationConfig()->ShortDateFormat);
         else if (Variables[i].VarName == "TYR") VarName = QString("%1").arg(QDate::currentDate().year());
         else if (Variables[i].VarName == "TMD") VarName = ito2a(QDate::currentDate().month());
         else if (Variables[i].VarName == "TDY") VarName = ito2a(QDate::currentDate().day());
         //else if (Variables[i].VarName == "TMM") VarName = UpInitials(QDate::currentDate().longMonthName(QDate::currentDate().month()));
         //else if (Variables[i].VarName == "TDW") VarName = UpInitials(QDate::currentDate().longDayName(QDate::currentDate().dayOfWeek()));
         else if (Variables[i].VarName == "TMM") VarName = loc.monthName(QDate::currentDate().month());
         else if (Variables[i].VarName == "TDW") VarName = loc.dayName(QDate::currentDate().dayOfWeek());

         else VarName = "";

         // Replace variable with varname converted to html
         if (!VarName.isEmpty()) 
            SourceText.replace("%" + Variables[i].VarName + "%", HTMLConverter()->ToHTML(VarName));
      }
   return SourceText;
}

// ************************************************

bool cVariable::IsTextHaveVariables(QString SourceText) 
{
    if (SourceText.isEmpty()) 
    return false;

    for (int i = 0; i < Variables.count(); i++) 
      if (SourceText.contains("%"+Variables[i].VarName+"%")) 
         return true;
    return false;
}

bool cVariable::IsObjectHaveVariables(cDiaporamaObject *Object) 
{
    //for (int i = 0; i < Object->ObjectComposition.compositionObjects.count();i++)
    //    if (IsTextHaveVariables(Object->ObjectComposition.compositionObjects[i]->tParams.Text))
    //        return true;

    for(auto const CompositionObject : Object->ObjectComposition.compositionObjects)
       if (IsTextHaveVariables(CompositionObject->tParams.Text))
          return true;
    return false;
}

//************************************************

void AppendAction(QMenu *Menu,QWidget *ParentWindow,QString Title) 
{
   QAction* Action = new QAction(Title, ParentWindow);
   Action->setIconVisibleInMenu(true);
   //Action->setFont(QFont("Sans Serif",9));
   Action->setFont(getAppFont(Sans9));
   Action->setToolTip(QApplication::translate("Variables", "Select this variable"));
   Menu->addAction(Action);
}

QString cVariable::PopupVariableMenu(QWidget *ParentWindow) 
{
   QFont f = getAppFont(Sans9);
    QMenu   *ContextMenu    =new QMenu(ParentWindow);   ContextMenu->setFont(f);
    QMenu   *PropertiesMenu =new QMenu(ParentWindow);   PropertiesMenu->setFont(f);     PropertiesMenu->setTitle(QApplication::translate("Variables","Project properties values"));
    QMenu   *ProjectDateMenu=new QMenu(ParentWindow);   ProjectDateMenu->setFont(f);    ProjectDateMenu->setTitle(QApplication::translate("Variables","Project date values"));
    QMenu   *SlideMenu      =new QMenu(ParentWindow);   SlideMenu->setFont(f);          SlideMenu->setTitle(QApplication::translate("Variables","Current slide values"));
    QMenu   *ChapterMenu    =new QMenu(ParentWindow);   ChapterMenu->setFont(f);        ChapterMenu->setTitle(QApplication::translate("Variables","Chapters values"));
    QMenu   *ChapterDateMenu=new QMenu(ParentWindow);   ChapterDateMenu->setFont(f);    ChapterDateMenu->setTitle(QApplication::translate("Variables","Current chapter date values"));
    QMenu   *VariousMenu    =new QMenu(ParentWindow);   VariousMenu->setFont(f);        VariousMenu->setTitle(QApplication::translate("Variables","Various values"));
    QMenu   *TodayDateMenu  =new QMenu(ParentWindow);   TodayDateMenu->setFont(f);      TodayDateMenu->setTitle(QApplication::translate("Variables","Today date values"));
    QString Var="";

    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project title")+"\t%PTT%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project author")+"\t%PAU%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project author as <Author presents>")+"\t%PAI%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project author as <A Author production>")+"\t%PAP%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project album")+"\t%PAL%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project location name")+"\t%PLN%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project location address")+"\t%PLA%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project comment")+"\t%PCT%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project duration")+"\t%PDD%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Project slide count")+"\t%PSC%");

    AppendAction(ProjectDateMenu,ParentWindow,QApplication::translate("Variables","Long date")+"\t%PLD%");
    AppendAction(ProjectDateMenu,ParentWindow,QApplication::translate("Variables","Short date")+"\t%PSD%");
    AppendAction(ProjectDateMenu,ParentWindow,QApplication::translate("Variables","Year")+"\t%PYR%");
    AppendAction(ProjectDateMenu,ParentWindow,QApplication::translate("Variables","Month")+"\t%PMM%");
    AppendAction(ProjectDateMenu,ParentWindow,QApplication::translate("Variables","Month as number")+"\t%PMD%");
    AppendAction(ProjectDateMenu,ParentWindow,QApplication::translate("Variables","Day")+"\t%PDY%");
    AppendAction(ProjectDateMenu,ParentWindow,QApplication::translate("Variables","Day of week")+"\t%PDW%");

    AppendAction(SlideMenu,ParentWindow,QApplication::translate("Variables","Current slide name")+"\t%CSN%");
    AppendAction(SlideMenu,ParentWindow,QApplication::translate("Variables","Current slide number")+"\t%CSR%");

    AppendAction(ChapterMenu,ParentWindow,QApplication::translate("Variables","Current chapter name")+"\t%CCN%");
    AppendAction(ChapterMenu,ParentWindow,QApplication::translate("Variables","Current chapter number")+"\t%CCR%");
    AppendAction(ChapterMenu,ParentWindow,QApplication::translate("Variables","Current chapter number as \"Chapter N°\"")+"\t%CCI%");
    AppendAction(ChapterMenu,ParentWindow,QApplication::translate("Variables","Current chapter duration")+"\t%CCD%");
    AppendAction(ChapterMenu,ParentWindow,QApplication::translate("Variables","Chapter count")+"\t%CCT%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Current chapter location name")+"\t%CLN%");
    AppendAction(PropertiesMenu,ParentWindow,QApplication::translate("Variables","Current chapter location address")+"\t%CLA%");

    AppendAction(ChapterDateMenu,ParentWindow,QApplication::translate("Variables","Long date")+"\t%CLD%");
    AppendAction(ChapterDateMenu,ParentWindow,QApplication::translate("Variables","Short date")+"\t%CSD%");
    AppendAction(ChapterDateMenu,ParentWindow,QApplication::translate("Variables","Year")+"\t%CYR%");
    AppendAction(ChapterDateMenu,ParentWindow,QApplication::translate("Variables","Month")+"\t%CMM%");
    AppendAction(ChapterDateMenu,ParentWindow,QApplication::translate("Variables","Month as number")+"\t%CMD%");
    AppendAction(ChapterDateMenu,ParentWindow,QApplication::translate("Variables","Day")+"\t%CDY%");
    AppendAction(ChapterDateMenu,ParentWindow,QApplication::translate("Variables","Day of week")+"\t%CDW%");

    AppendAction(VariousMenu,ParentWindow,QApplication::translate("Variables","ffDiaporama version")+"\t%FFD%");
    AppendAction(VariousMenu,ParentWindow,QApplication::translate("Variables","Copyright")+"\t%CPY%");
    AppendAction(VariousMenu,ParentWindow,QApplication::translate("Variables","The end")+"\t%END%");
    AppendAction(VariousMenu,ParentWindow,QApplication::translate("Variables","Project made")+"\t%STA%");
    AppendAction(VariousMenu,ParentWindow,QApplication::translate("Variables","Content")+"\t%STP%");
    AppendAction(VariousMenu,ParentWindow,QApplication::translate("Variables","Musical content")+"\t%STM%");

    AppendAction(TodayDateMenu,ParentWindow,QApplication::translate("Variables","Long date")+"\t%TLD%");
    AppendAction(TodayDateMenu,ParentWindow,QApplication::translate("Variables","Short date")+"\t%TSD%");
    AppendAction(TodayDateMenu,ParentWindow,QApplication::translate("Variables","Year")+"\t%TYR%");
    AppendAction(TodayDateMenu,ParentWindow,QApplication::translate("Variables","Month")+"\t%TMM%");
    AppendAction(TodayDateMenu,ParentWindow,QApplication::translate("Variables","Month as number")+"\t%TMD%");
    AppendAction(TodayDateMenu,ParentWindow,QApplication::translate("Variables","Day")+"\t%TDY%");
    AppendAction(TodayDateMenu,ParentWindow,QApplication::translate("Variables","Day of week")+"\t%TDW%");

    ContextMenu->addMenu(PropertiesMenu);
    PropertiesMenu->addMenu(ProjectDateMenu);
    ContextMenu->addMenu(SlideMenu);
    ContextMenu->addMenu(ChapterMenu);
    ChapterMenu->addMenu(ChapterDateMenu);
    ContextMenu->addMenu(VariousMenu);
    VariousMenu->addMenu(TodayDateMenu);

    QAction *Ret=ContextMenu->exec(QCursor::pos());
    if (Ret) {
        Var=Ret->text();
        Var=Var.mid(Var.indexOf("%"));
    }
    delete ContextMenu;
    return Var;
}
