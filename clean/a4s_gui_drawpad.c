#include <ava_sdk.h>
#include <ava_hisdk.h>
#include <a4s.h>
#include <ava_windows.h>
#include <a4s_gui.h>
#include <a4s_cue.h>
#include <language.h>

#include "a4s_gui_drawpad.h"
#include "a4s_gui_rawdat.h"
#include <stdio.h>

#define DEBUG 0
#define ISSAVE 0
#define DRAWLOCALUI 0
#define FIXDELAY 1

#define BLUE 0x12eb74 #define RED 0x3663eb #define ARGB1555_RED 0xfc00
#define ARGB1555_GREEN 0x83e0
#define ARGB1555_BLUE 0x801f

static PPTINFO ppt;
static unsigned int gColor = ARGB1555_GREEN;
static unsigned int gControlMask = 0x0;

static int init4bitPageButton(struct WindowsStruct * windows, struct WindowStruct * win)
{
    GUI_DATA * GUIData = windows->SendMessageData;
    A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
    DRAWPAD_DATA * drawdata = &A4SGUIData->drawpad.data;

    if(drawdata->en_4bit == 0)
        return -1;

    int y;

    char *bufferY = HI_MPI_SYS_Mmap(drawdata->pic4bitVoBuf.stVFrame.u32PhyAddr[0], 1920/8*1080);
    for(y=BTNL_Y/2; y<(BTNL_Y/2)+30; y++)
    {
        memcpy(bufferY+480*y+(BTNL_X/4),               btn_left+(y- (BTNL_Y/2) )*20,                sizeof(char)*20);
    }

    for(y=(BTNR_Y/2); y<(BTNR_Y/2)+30; y++)
    {
        memcpy(bufferY+480*y+(BTNR_X/4), btn_right+(y-(BTNR_Y/2))*20, sizeof(char)*20);
    }

    for(y=(CLEANBT_Y/2); y<(CLEANBT_Y/2)+30; y++)
    {
        memcpy(bufferY+480*y+(CLEANBT_X/4), btn_clean+(y-(CLEANBT_Y/2))*30, sizeof(char)*30);
    }

    HI_MPI_SYS_Munmap(bufferY, 1920/8*1080);

    return 0;
}

static int cleanDrawpad(struct WindowsStruct * windows, struct WindowStruct * win)
{
    clean4BitBuffer(windows, win, 1920, 1080);     cleanInterDrawpadBuffer(windows, win, 1920, 1080);     init4bitPageButton(windows, win);
    return 0;
}

static int  drawSomething( struct WindowsStruct *windows,  struct WindowStruct * win, int type, int eventID, int mx, int my)
{
    char msgbuf[512] = "";

    GUI_DATA * GUIData;
    GUIData = windows->SendMessageData;
    A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
    int x=0, y=0;
    BOOL isPressBtn = 0;

    pDrawPadInfo drawinfo = &A4SGUIData->drawpad.info;
    DRAWPAD_DATA *drawdata = &A4SGUIData->drawpad.data;

    x = mx; y = my;
#if DEBUG
    printf("%s-- type=%d\n", __FUNCTION__, type);
#endif
    switch(eventID)
    {
        case CMD_LEFTBUTTONRELEASED :
            drawinfo->mouse[type].pressed = 0;
            drawinfo->mouse[type].last_x = -100;
            drawinfo->mouse[type].last_y = -100;

            if( gettickcount()-ppt.timval > 200)            {
                init4bitPageButton(windows, win);
                                if(x>BTNL_X && x<BTNL_X+80 && y>BTNL_Y && y<BTNL_Y+60
                        && drawinfo->mouse[type].pressedX>BTNL_X && drawinfo->mouse[type].pressedX<BTNL_X+80
                        && drawinfo->mouse[type].pressedY>BTNL_Y && drawinfo->mouse[type].pressedY<BTNL_Y+60)
                {
                    snprintf(msgbuf, sizeof(msgbuf)-1,"WHATPPTPAGE");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_TMI, COMMAND_BY_CHAR,
                                    msgbuf, strlen(msgbuf) + 1);
                    usleep(200000);
                    if(ppt.currentpage == 0)
                        return -1;
#if ISSAVE
                    a4s_gui_drawpad_SaveCurrentPage(windows, win, ppt.currentpage, 1920, 1080);#endif

                    snprintf(msgbuf, sizeof(msgbuf)-1,"PPTPREV");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_TMI, COMMAND_BY_CHAR,
                                    msgbuf, strlen(msgbuf) + 1);
                    isPressBtn = 1;
                }

                                if(x>BTNR_X && x<BTNR_X+80 && y>BTNR_Y && y<BTNR_Y+60
                        && drawinfo->mouse[type].pressedX>BTNR_X && drawinfo->mouse[type].pressedX<BTNR_X+80
                        && drawinfo->mouse[type].pressedY>BTNR_Y && drawinfo->mouse[type].pressedY<BTNR_Y+60)
                {
                    snprintf(msgbuf, sizeof(msgbuf)-1,"WHATPPTPAGE");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_TMI, COMMAND_BY_CHAR,
                                    msgbuf, strlen(msgbuf) + 1);
                    usleep(200000);
                    if(ppt.currentpage == 0)
                        return -1;
                    printf("ppt. currentpage=%d\n", ppt.currentpage);#if ISSAVE
                    a4s_gui_drawpad_SaveCurrentPage(windows, win, ppt.currentpage, 1920, 1080);
#endif

                    snprintf(msgbuf, sizeof(msgbuf)-1,"PPTNEXT");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_TMI, COMMAND_BY_CHAR,
                                    msgbuf, strlen(msgbuf) + 1);
                    isPressBtn = 1;
                }

                                if(x>CLEANBT_X && x<CLEANBT_X+120 && y>CLEANBT_Y && y<CLEANBT_Y+60
                        && drawinfo->mouse[type].pressedX>CLEANBT_X && drawinfo->mouse[type].pressedX<CLEANBT_X+120
                        && drawinfo->mouse[type].pressedY>CLEANBT_Y && drawinfo->mouse[type].pressedY<CLEANBT_Y+60)
                {
                    cleanDrawpad(windows, win);
                    isPressBtn = 1;
                }
            }

            if(isPressBtn)
            {
                ppt.timval = gettickcount();             }

            break;
        case CMD_LEFTBUTTONPRESSED :
            drawinfo->mouse[type].pressed = 1;
            drawinfo->mouse[type].last_x = x;
            drawinfo->mouse[type].last_y = y;
            drawinfo->mouse[type].pressedX = x;
            drawinfo->mouse[type].pressedY = y;
#if DRAWLOCALUI
            drawPoint(windows, x+win->basicInfo.rect.s32X,
                            y+win->basicInfo.rect.s32Y,
                            2, 0xff0012ff, 0xff0012ff);
#endif

            if(drawdata->en_4bit)
                AVA_GUI_DrawPointByFormat(AVA_DRAWPAD_MODE_4BIT, drawdata->pic4bitVoBuf.stVFrame.u32PhyAddr, &win->basicInfo.rect,
                                     &win->basicInfo.rect,
                                     x+win->basicInfo.rect.s32X,
                                     y+win->basicInfo.rect.s32Y,
                                     0x0a, 3, 1920, 1080);

            if(drawdata->en_inter)
                AVA_GUI_DrawPointByFormat(INTERA_DRAWPAD_MODE, drawdata->InterDrawpadVoBuf.stVFrame.u32PhyAddr, &win->basicInfo.rect,
                                     &win->basicInfo.rect,
                                     x+win->basicInfo.rect.s32X,
                                     y+win->basicInfo.rect.s32Y,
                                     gColor, 5, 1920, 1080);
            break;
        case CMD_MOUSEMOVE:
            if(drawinfo->mouse[type].pressed == 1)
            {
                if(x>=0 && y>=0)
                {

                    if(drawdata->en_4bit)
                        AVA_GUI_DrawLineByFormat(drawdata->pic4bitVoBuf.stVFrame.u32PhyAddr, 1920, 1080,
                                    AVA_DRAWPAD_MODE_4BIT, &win->basicInfo.rect, &win->basicInfo.rect, drawinfo->mouse[type].last_x, drawinfo->mouse[type].last_y,
                                    x, y, 0x0a, 3);

                    if(drawdata->en_inter)
                        AVA_GUI_DrawLineByFormat(drawdata->InterDrawpadVoBuf.stVFrame.u32PhyAddr, 1920, 1080,
                                    INTERA_DRAWPAD_MODE, &win->basicInfo.rect, &win->basicInfo.rect, drawinfo->mouse[type].last_x, drawinfo->mouse[type].last_y,
                                    x, y, gColor, 5);

                    drawinfo->mouse[type].last_x = x;
                    drawinfo->mouse[type].last_y = y;
                }
            }
            break;
        default:
            printf("draw text\n");
            break;
    }
    return 0;
}

static int gui_drawpadOverlay(GUI_DATA * GUIData, int rtspNum)
{
    char msgbuf[512] = "";
    sprintf(msgbuf, "drawpad set %d", rtspNum);
    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                    msgbuf, strlen(msgbuf) + 1);
    return 0;
}

static int setMULRemoteDrawpad(A4SGui_Data * A4SGUIData, GUI_DATA * GUIData,
                            const char *username, const char *localname, const int rtspNum)
{
    pDrawPadInfo pdrawinfo = &A4SGUIData->drawpad.info;
    char msgbuf[512] = "";

    printf("enter MULTIPLE,%d, %s\n", rtspNum, username);

    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 3");    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad switchHDMI");
    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad set");
    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad copylock");
    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad init");
    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

    return 0;
}


static int setRemoteDrawpad(A4SGui_Data * A4SGUIData, GUI_DATA * GUIData,
                            const char *username, const char *localname, const int rtspNum)
{
    pDrawPadInfo pdrawinfo = &A4SGUIData->drawpad.info;
    char msgbuf[512] = "";

    switch(pdrawinfo->mode)
    {
        case SINGLE:
            printf("name = %s, localname=%s\n", username, localname);
            sprintf(msgbuf, "%s/MSG %d %s %d %s", username, MODULE_KMM, "drawpad remotemouse", rtspNum, localname);
            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

            sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 32");
            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
            usleep(100);

            sprintf(msgbuf, "%s/MSG %d %s %s", username, MODULE_KMM, "drawpad status 1", localname);
            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
        break;
        case MULTIPLE:
            printf("enter MULTIPLE,%d, %s\n", rtspNum, username);
            sprintf(msgbuf, "drawpad remotemouse %d %s", rtspNum, username);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                            msgbuf, strlen(msgbuf) + 1);

            sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 4");
            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
        break;
    }

#if  FIXDELAY
    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad switchHDMI");
    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad set");
    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad copylock");
    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
#endif

    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad init");
    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

    return 0;
}

int drawpadMain(struct WindowsStruct * windows, struct WindowStruct * win, void *indata)
{
    int type, i;
    char msgbuf[512] = " ", username[128]="", localname[128]="";
    char cmd[64] = "", option1[64] = "", option2[64] = "", option3[64] = "", option4[64] = "";
    GUI_DATA * GUIData = windows->SendMessageData;
    A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
    const pDrawPadInfo pdrawinfo = &A4SGUIData->drawpad.info;

    sscanf(indata, "%[^ ] %[^ ] %[^ ] %[^ ] %s", cmd, option1, option2, option3, option4);

    GetConfigData(GUIData, "rtsp", "loginidstun", msgbuf, sizeof(msgbuf));
    ava_mtargs(msgbuf, "usr", localname, sizeof(localname));

    if(!strcasecmp(cmd, "open"))
    {
        if(A4SGUIData->drawpad.info.winopened == 0)
        {
            for(type=0; type<10; type++)
            {
                A4SGUIData->drawpad.info.mouse[type].last_x = -100;
                A4SGUIData->drawpad.info.mouse[type].last_y = -100;
                A4SGUIData->drawpad.info.mouse[type].pressed = 0;             }
            A4SGUIData->drawpad.info.winopened = 1;
            ppt.currentpage = 0;
            ppt.timval = gettickcount();
            init4bitPageButton(windows, win);
        }
    }
    else if(!strcasecmp(cmd, "close"))
    {
        A4SGUIData->drawpad.info.winopened = 0;
        for (type = 0; type < 10; type++)
        {
            A4SGUIData->drawpad.info.mouse[type].pressed = 0;         }
    }
    else if(!strcasecmp(cmd, "mode"))
    {
        if(!strcasecmp(option1, "single"))
        {
            pdrawinfo->mode = SINGLE;
        }
        else if(!strcasecmp(option1, "multiple"))
        {
            pdrawinfo->mode = MULTIPLE;
        }
    }
    else if(!strcasecmp(cmd, "other"))
    {
        if(!strcasecmp(option1, "save"))
        {
            a4s_gui_drawpad_SaveCurrentPage(windows, win, ppt.currentpage, 1920, 1080);
        }
        else if(!strcasecmp(option1, "init"))
        {
            ppt.currentpage = 0;
            cleanDrawpad(windows, win);
        }
                else if(!strcasecmp(option1, "key"))
        {
           if(!strcasecmp(option2, "up") )
           {
               snprintf(msgbuf, sizeof(msgbuf)-1,"PPTPREV");
               AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_TMI, COMMAND_BY_CHAR,
                               msgbuf, strlen(msgbuf) + 1);
           }
           else if(!strcasecmp(option2, "down") )
           {
               snprintf(msgbuf, sizeof(msgbuf)-1,"PPTNEXT");
               AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_TMI, COMMAND_BY_CHAR,
                               msgbuf, strlen(msgbuf) + 1);
           }
        }
                else if(!strcasecmp(option1, "ctrlppt"))
        {
           if(!strcasecmp(option2, "pptpage") )
           {
               printf("page = %d\n", atoi(option3));
               ppt.currentpage = atoi(option3);
           }
           else if(!strcasecmp(option2, "pptpage_change"))
           {
               printf("chpage = %d\n", atoi(option3));
               ppt.currentpage = atoi(option3);
               cleanDrawpad(windows, win);
#if ISSAVE
               a4s_gui_drawpad_GetDataWithPage(windows, win, ppt.currentpage, 1920, 1080);
#endif
           }
           else if(!strcasecmp(option2, "pptend"))
           {
               cleanDrawpad(windows, win);
               deleteCacheData(windows, win);
           }
        }        else if (!strcasecmp(option1, "color"))
        {
            gColor = strtoul(option2,0,0);
        }
        else if(!strcasecmp(option1, "chooseRtsp"))        {
            int rtspNum = atoi(option3) - 4;
            if (getRemoteUser(GUIData, username, rtspNum) < 0)
            {
                printf("%d cant get name\n", __LINE__);
                return -1;
            }

            printf("enter chooseRtsp option2=%s, rtspNum=%d, username=%s\n",
                   option2, rtspNum, username);


            
            gControlMask = 0x4;

            snprintf(msgbuf, sizeof(msgbuf)-1,"drawpad status 3");
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                            msgbuf, strlen(msgbuf) + 1);
            pdrawinfo->isSpeakerDraw[rtspNum-1] = 1;

            if(pdrawinfo->isDrawRTSP[rtspNum-1] == 0)
            {
                sprintf(msgbuf, "%s/MSG %d %s %d", username, MODULE_GUI, "drawpad setMask", (0x4<<(rtspNum-1)) );
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
            }
            else
            {
                sprintf(msgbuf, "%s/MSG %d %s %d", username, MODULE_GUI, "drawpad setMask", (0x4<<(rtspNum-1)) + 3 );
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
            }

            sprintf(msgbuf, "drawpad remotemouse %d %s", rtspNum, username);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                            msgbuf, strlen(msgbuf) + 1);



        }
        else if(!strcasecmp(option1, "deChooseRtsp"))
        {
            int i;
            printf("enter DeChooseRtsp\n");

            for(i=0; i<3; i++)
            {
                if (getRemoteUser(GUIData, username, i+1) < 0 || strlen(localname)<1)
                {
                    printf(" %s %d cant get name\n", __FILE__, __LINE__);
                    continue;
                }
                pdrawinfo->isSpeakerDraw[i] = 0;

                if(pdrawinfo->isDrawRTSP[i] == 0)
                {
                    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 0");
                    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
                }
                else
                {
                    sprintf(msgbuf, "%s/MSG %d %s %d", username, MODULE_GUI, "drawpad setMask", (0x4<<i));
                    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
                }
            }

            sprintf(msgbuf, "drawpad remotemouse");
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                            msgbuf, strlen(msgbuf) + 1);

            snprintf(msgbuf, sizeof(msgbuf)-1,"drawpad setMask 3");
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                            msgbuf, strlen(msgbuf) + 1);

        }
        else if(!strcasecmp(option1, "MultipleModeControl"))
        {
            int i;
            const char* isOn = option2;            int userNum = atoi(option3);            int rtspNum = 0;

            for(i=0; i<3; i++)
            {
                if( (userNum >> i) & 0x01)
                {
                    rtspNum = i+1;
                    break;
                }
            }

            if (getRemoteUser(GUIData, username, rtspNum) < 0 || strlen(localname)<1)
            {
                printf("%d cant get name\n", __LINE__);
                return -1;
            }

            if(!strcasecmp(isOn, "on"))
            {
                setMULRemoteDrawpad(A4SGUIData, GUIData, username, localname, rtspNum);

                if(pdrawinfo->isSpeakerDraw[rtspNum-1] == 1)
                {
                    sprintf(msgbuf, "%s/MSG %d %s %d", username, MODULE_GUI, "drawpad setMask", (0x4<<(rtspNum-1)) + 3 );                    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
                }

                pdrawinfo->isDrawRTSP[rtspNum-1] = 1;
            }
            else if(!strcasecmp(isOn, "off"))
            {

                sprintf(msgbuf, "drawpad setMask 3");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                msgbuf, strlen(msgbuf) + 1);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad switchRTSP1");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);


                sprintf(msgbuf, "%s/MSG %d %s %d", username, MODULE_GUI, "drawpad setMask", 0x4<<(rtspNum-1) );
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                                pdrawinfo->isDrawRTSP[rtspNum-1] = 0;
            }
        }
        else if(!strcasecmp(option1, "restoreSubdrawpad"))
        {
                        for(i=0; i<3; i++)
            {
                if (getRemoteUser(GUIData, username, i+1) < 0 || strlen(localname)<1)
                {
                    printf(" %s %d cant get name\n", __FILE__, __LINE__);
                    continue;
                }

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 0");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad SWITCHRTSP1");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad unlock");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad unset");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                pdrawinfo->isDrawRTSP[i] = 0;
                pdrawinfo->isSpeakerDraw[i] = 0;
            }
        }
        else if(!strcasecmp(option1, "setMaskByBit"))
        {

        }
        else if(!strcasecmp(option1, "setMask"))
        {
            gControlMask = strtoul(option2,0,0);
        }
        else if(!strcasecmp(option1, "controlmask"))
        {
            gControlMask = strtoul(option2,0,0);
            printf("get ControlMask=%#x\n", gControlMask);

            if(gControlMask & 0x04)
            {
                if (getRemoteUser(GUIData, username, 1) < 0 || strlen(localname)<1)
                {
                    printf("%d cant get name\n", __LINE__);
                    return -1;
                }
                setRemoteDrawpad(A4SGUIData, GUIData, username, localname, 1);
            }
            else if(gControlMask & 0x08)
            {
                if (getRemoteUser(GUIData, username, 2) < 0 || strlen(localname)<1)
                {
                    printf("%d cant get name\n", __LINE__);
                    return -1;
                }

                setRemoteDrawpad(A4SGUIData, GUIData, username, localname, 2);
            }
            else if(gControlMask & 0x10)
            {
                if (getRemoteUser(GUIData, username, 3) < 0 || strlen(localname)<1)
                {
                    printf("%d cant get name\n", __LINE__);
                    return -1;
                }

                setRemoteDrawpad(A4SGUIData, GUIData, username, localname, 3);
            }
            else if(gControlMask & 0x02)             {
                printf("%s %d close all mask=%#x\n", __FILE__, __LINE__, gControlMask);
                for (i = 0; i < 3; i++)
                {
                    printf("close i+1=%d\n", i + 1);
                    if (getRemoteUser(GUIData, username, i + 1) < 0 || strlen(localname) < 1)
                        continue;

                    switch(pdrawinfo->mode)
                    {
                        case MULTIPLE:
                            sprintf(msgbuf, "drawpad remotemouse");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                            msgbuf, strlen(msgbuf) + 1);

                            sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 1");
                            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                            sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad unset");
                            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
                        break;

                        case SINGLE:
                            sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_KMM, "drawpad remotemouse");
                            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                        break;
                    }
#if FIXDELAY


                    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad switchRTSP1");
                    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
#endif
                }
            }

           /*
            if(gControlMask & 0x04)             {
                if (getRemoteUser(GUIData, username, 1) < 0 || strlen(localname)<1)
                {
                    printf("%d cant get name\n", __LINE__);
                    return -1;
                }

                switch(pdrawinfo->mode)
                {
                    case SINGLE:
                        printf("name = %s, localname=%s\n", username, localname);
                        sprintf(msgbuf, "%s/MSG %d %s %s", username, MODULE_KMM, "drawpad remotemouse 1", localname);
                        a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                        sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 1");
                        a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
                    break;
                    case MULTIPLE:
                        sprintf(msgbuf, "drawpad remotemouse 1 %s", username);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                        msgbuf, strlen(msgbuf) + 1);

                        sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 4");
                        a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
                    break;
                }
#if  FIXDELAY
                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad switchHDMI");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad set");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad copylock");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
#endif

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad init");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
            }
            else if(gControlMask & 0x08)            {
                if (getRemoteUser(GUIData, username, 2) < 0 || strlen(localname)<1)
                {
                    printf("%d cant get name\n", __LINE__);
                    return -1;
                }

                sprintf(msgbuf, "%s/MSG %d %s %s", username, MODULE_KMM, "drawpad remotemouse 2", localname);
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad controlmask 1");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad init");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
            }
            else if(gControlMask & 0x10)            {
                if (getRemoteUser(GUIData, username, 3) < 0 || strlen(localname) < 1)
                {
                    printf("%d cant get name\n", __LINE__);
                    return -1;
                }

                sprintf(msgbuf, "%s/MSG %d %s %s", username, MODULE_KMM, "drawpad remotemouse 3", localname);
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad controlmask 1");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad init");
                a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
            }
            else if(gControlMask & 0x02)             {
                printf("%s %d close all mask=%#x\n", __FILE__, __LINE__, gControlMask);
                for (i = 0; i < 3; i++)
                {
                    printf("close i+1=%d\n", i + 1);
                    if (getRemoteUser(GUIData, username, i + 1) < 0 || strlen(localname) < 1)
                        continue;

                    switch(pdrawinfo->mode)
                    {
                        case MULTIPLE:
                            sprintf(msgbuf, "drawpad remotemouse");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                            msgbuf, strlen(msgbuf) + 1);

                            sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "drawpad setMask 1");
                            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
                        break;

                        case SINGLE:
                            sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_KMM, "drawpad remotemouse");
                            a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                        break;
                    }
#if FIXDELAY
                    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad unset");
                    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);

                    sprintf(msgbuf, "%s/MSG %d %s", username, MODULE_GUI, "subdrawpad switchRTSP1");
                    a4s_module_function(MODULE_RSC, MODULE_GUI, COMMAND_RSC_SEND_MSG, msgbuf);
#endif
                }
            }
            */
        }
    }
        else if(!strcasecmp(cmd, "mouse"))
    {
        type = atoi(option4);

        if( (gControlMask >> (type-1) ) & 0x01 )             drawSomething(windows, win, atoi(option4), atoi(option3), atoi(option1), atoi(option2));
    }
    return 0;
}
