#ifndef _PLAYGAME_H_
#define _PLAYGAME_H_

GtkWidget *gamewin, *vbox, *pbyp, *grview, *vscrollbar, *separator, *canbutton, *speedupbutton, *slowdownbutton,
          *pbptext, *prpbpbutton, *boxbutton, *dlgBox, *pitcherpicwin, *batterpicwin;
GtkLabel *innlabel[10], *innrlabel[10][2], *totrlabel[2], *tothlabel[2], *totelabel[2], *pitcherlabel, *outlabel,
         *outslabel, *halfilabel, *ilabel, *batterlabel, *firstlabel, *secondlabel, *thirdlabel, *decklabel, *holelabel, *msglabel[4];

gchar cresp, savepitchername[100], savebattername[100], savepitcherdob[9], savebatterdob[9];
gint offdialsw, defdialsw, lineupsw, clineupsw, EOSsw, EOPSsw, pitpicwinlocX, pitpicwinlocY, batpicwinlocX, batpicwinlocY;

#endif

