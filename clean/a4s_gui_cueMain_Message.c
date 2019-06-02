#include <ava_sdk.h>
#include <ava_hisdk.h>
#include <a4s.h>
#include <ava_windows.h>
#include <a4s_gui.h>
#include <a4s_cue.h>
#include <language.h>
#include "hi_common.h"
#include "hi_comm_video.h"
#include <mpi_vpss.h>
#include <sys/stat.h>
#include <a4s_vps.h>
#include "../tools/a4s_gui_fpga.h"

#define DRAWDEBUG 1
#define ARGBto1555(x)   (((((x) & (0xff << 24))?1:0 ) << 15) | (((x) & (0xf8 << 16)) >> 9) | (((x) & (0xf8 << 8)) >> 6) | (((x) & 0xf8) >> 3))

static int deinitDrawPadBuffer(DRAWPAD *drawpad)
{
    DRAWPAD_DATA *drawdata = &drawpad->data;
    if(drawdata->pic4bitVbBlk != -1)
        HI_MPI_VB_ReleaseBlock(drawdata->pic4bitVbBlk);
    if (drawdata->InterDrawpadVbBlk != -1)
        HI_MPI_VB_ReleaseBlock(drawdata->InterDrawpadVbBlk);
    return 0;
}

static int a4s_drawpad_set_data(GUI_DATA * GUIData, int signalId)
{
    A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;

    if(A4SGUIData->DrawPadEnable != 1 || A4SGUIData->drawpad.data.en_inter != 1)
        return -1;
    if(signalId < 0 || signalId >= GUIData->product->videoSignalInfo.videoResourceCount)
        return -1;

    VIDEO_FRAME_INFO_S VoBuf;
    VoBuf = A4SGUIData->drawpad.data.InterDrawpadVoBuf;

    printf("====a4s_drawpad_set_data====%d===enPixelFormat===%d==\n", signalId, VoBuf.stVFrame.enPixelFormat);

    VRM_Command_SetWhiteBoard_Data vsd;
    vsd.signalNo = signalId;
    vsd.whiteBoardData = VoBuf;
    a4s_module_function( MODULE_VRM, MODULE_VRM, COMMAND_VRM_LOCK_SET_WHITEBOARD_DATA, &vsd);

    return 0;
}

static int a4s_drawpad_unset_data(GUI_DATA * GUIData, int signalId)
{
    A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;

    if(A4SGUIData->DrawPadEnable != 1 || A4SGUIData->drawpad.data.en_inter != 1)
        return -1;
    if(signalId < 0 || signalId >= GUIData->product->videoSignalInfo.videoResourceCount)
        return -1;

    VIDEO_FRAME_INFO_S VoBuf;
    memset(&VoBuf, 0, sizeof(VIDEO_FRAME_INFO_S));

    VRM_Command_SetWhiteBoard_Data vsd;
    vsd.signalNo = signalId;
    vsd.whiteBoardData = VoBuf;
    a4s_module_function( MODULE_VRM, MODULE_VRM, COMMAND_VRM_LOCK_SET_WHITEBOARD_DATA, &vsd);

    return 0;
}

static int executeGUICommandControl(GUI_DATA * GUIData, char *command)
{
    A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;

    int i;
    char *param;
    char group[128] = "";
    char item[128] = "";

    ava_mtsubs(command, ' ', 1, group, sizeof(group));
    ava_mtsubs(command, ' ', 2, item, sizeof( item));
    param = command + strlen(group) + strlen( item) + 2;
    if (!strcasecmp( group, "usb"))
    {
        if(A4SGUIData->LCDenable == 1)
        {
            char sbuf[64] = "";
            sprintf(sbuf, "disp %s %s", item, param);
            A4SGUIData->lcddisptime = 10;
            if(strcasecmp(A4SGUIData->lcddispbuf, sbuf+5))
            {
                strcpy(A4SGUIData->lcddispbuf, sbuf+5);
                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_LCD,  COMMAND_BY_CHAR,
                                sbuf,  strlen( sbuf) + 1 );
            }
        }
    }
    else if ( !strcasecmp( group, "avalogo"))
    {
        char msgbuf[64] = "";
        sprintf(msgbuf, "pc gui %s", command);
        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                        msgbuf,  strlen( msgbuf)+1);
    }
    else if ( !strcasecmp( group, "NetworkParam"))
    {
        char sbuf[64] = "";
        if(atoi(item) >= 1024 && atoi(item) <= 4096)
            A4SGUIData->NetworkParam = atoi(item) / 512 *512;
        sprintf(sbuf, "pc gui NetworkParam %d", A4SGUIData->NetworkParam);
        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                        sbuf,  strlen( sbuf)+1);
    }
    else if ( !strcasecmp( group, "KeyingEnable"))
    {
        if(A4SGUIData->KeyingEnable != 1)
            return 0;
        if(!a4s_keying_matter( command, A4SGUIData->keyingmask))
        {
        	char msgbuf[128] = "";
            sprintf(msgbuf, "pc gui %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_HTTP, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
            sprintf(msgbuf, "sendCmd %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_MCU, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
    }
    else    if ( !strncasecmp( group, "KeyingThreshold", strlen("KeyingThreshold")))
    {
        if(A4SGUIData->KeyingEnable != 1)
            return 0;
        if(!a4s_keying_threshold( command, A4SGUIData->keyingmask))
        {
            char msgbuf[64] = "";
            sprintf(msgbuf, "pc gui %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
    }
    else    if ( !strncasecmp( group, "KeyingBG", strlen("KeyingBG")))
    {
        if(A4SGUIData->KeyingEnable != 1)
            return 0;
        if(!a4s_keying_backgroundcolor( command, A4SGUIData->keyingmask))
        {
            char msgbuf[64] = "";
            sprintf(msgbuf, "pc gui %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
    }
    else    if ( !strcasecmp( group, "KeyingHDMI3Vo"))
    {
        if(A4SGUIData->KeyingEnable != 1)
            return 0;
        if(!a4s_keying_hdmi3vo( command))
        {
            char msgbuf[64] = "";
            sprintf(msgbuf, "pc gui %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
    }
    else    if ( !strcasecmp( group, "KeyingHDMI3VoColor"))
    {
        if(A4SGUIData->KeyingEnable != 1)
            return 0;
        if(!a4s_keying_hdmi3voColor( command))
        {
            char msgbuf[64] = "";
            sprintf(msgbuf, "pc gui %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
    }
    else    if ( !strcasecmp( group, "KeyingHDMI3VoML"))
    {
        if(A4SGUIData->KeyingEnable != 1)
            return 0;
        if(!a4s_keying_hdmi3voMainLuminance( command))
        {
            char msgbuf[64] = "";
            sprintf(msgbuf, "pc gui %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
    }
    else    if ( !strcasecmp( group, "KeyingHDMI3VoSL"))
    {
        if(A4SGUIData->KeyingEnable != 1)
            return 0;
        if(!a4s_keying_hdmi3voSubLuminance( command))
        {
            char msgbuf[64] = "";
            sprintf(msgbuf, "pc gui %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
    }
    else if (!strcasecmp(group, "blend"))
    {
        if ( !strcasecmp( item, "mode"))
        {
            if(GUIData->windows && GUIData->mainWindow)
                cueVideoSetBlendMode(GUIData->windows, GUIData->mainWindow, atoi(param));
        }
    }
    else if (!strcasecmp(group, "rec"))
    {
        if ( !strcasecmp( item, "info"))
        {
            char sbuf[64] = "";
            char msgbuf[64] = "";

            if(A4SGUIData->recBaseLength == 0 && A4SGUIData->recStartTime == -1)
            {
                strcpy(sbuf, "record null");
                strcpy(msgbuf, "pc rec stop");
                a4s_module_function(MODULE_RSC,MODULE_RSC,COMMAND_RSC_REPORT_INFO_BYMSG, "act6_stat='1'");

            }
            else if(A4SGUIData->recBaseLength > 0 && A4SGUIData->recStartTime == -1)
            {
                strcpy(sbuf, "record off");
                strcpy(msgbuf, "pc rec pause");
                a4s_module_function(MODULE_RSC,MODULE_RSC,COMMAND_RSC_REPORT_INFO_BYMSG, "act6_stat='1'");
            }
            else if(A4SGUIData->recStartTime > 0)
            {
                strcpy(sbuf, "record on");
                strcpy(msgbuf, "pc rec start");
                a4s_module_function(MODULE_RSC,MODULE_RSC,COMMAND_RSC_REPORT_INFO_BYMSG, "act5_stat='1'");
            }
            if(A4SGUIData->LCDenable == 1)
                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_LCD,  COMMAND_BY_CHAR,
                                sbuf,  strlen( sbuf) + 1 );
            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_KMM,  COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf) + 1 );
        }
    }
    else if (!strcasecmp(group, "rtsp"))
    {
        if ( !strcasecmp( item, "info"))
        {
            char sbuf[64] = "";

            if(A4SGUIData->rtspStartTime == -1)
            {
                strcpy(sbuf, "living null");
                a4s_module_function(MODULE_RSC,MODULE_RSC,COMMAND_RSC_REPORT_INFO_BYMSG, "act8_stat='1'");
            }
            else if(A4SGUIData->rtspStartTime > 0)
            {
                strcpy(sbuf, "living on");
                a4s_module_function(MODULE_RSC,MODULE_RSC,COMMAND_RSC_REPORT_INFO_BYMSG, "act7_stat='1'");
            }
            if(A4SGUIData->LCDenable == 1)
                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_LCD,  COMMAND_BY_CHAR,
                                sbuf,  strlen( sbuf) + 1 );
            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_KMM,  COMMAND_BY_CHAR,
                            sbuf,  strlen( sbuf) + 1 );
        }
    }
    else if (!strcasecmp(group, "ptz"))
    {
        if ( !strcasecmp( item, "getpresetzoom"))
        {
            char option1[64] = "";
            char option2[64] = "";
            char msgbuf[64] = "";
            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));
            ava_mtsubs( param, ' ', 2, option2, sizeof( option2));
            if(atoi(option1) >= 6 || atoi(option1) < 0 || atoi(option2) > 4 || atoi(option2) <= 0)
            {
                return 0;
            }
            sprintf( msgbuf, "%d", A4SGUIData->presetzoom[atoi(option1)][atoi(option2)-1]);
            gui_ptzControl_byWinID(GUIData, "setzoom", atoi(option1), msgbuf);
        }
        else if ( !strcasecmp( item, "setpresetzoom"))
        {
            char option1[64] = "";
            char option2[64] = "";
            char msgbuf[64] = "";
            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));
            ava_mtsubs( param, ' ', 2, option2, sizeof( option2));
            if(atoi(option1) >= 6 || atoi(option1) < 0 || atoi(option2) > 4 || atoi(option2) <= 0)
            {
                return 0;
            }
            A4SGUIData->presetzoom[atoi(option1)][atoi(option2)-1] = A4SGUIData->ptzZoomValue[atoi(option1)];
            sprintf( msgbuf, "pc gui presetzoom%d %d %d %d %d", atoi(option1)+1, A4SGUIData->presetzoom[atoi(option1)][0],
                A4SGUIData->presetzoom[atoi(option1)][1], A4SGUIData->presetzoom[atoi(option1)][2], A4SGUIData->presetzoom[atoi(option1)][3]);
            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_SYS,  COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf) + 1 );
        }
        else if ( !strcasecmp( item, "zoomin"))
        {
            char option1[64] = "";
            char msgbuf[64] = "";

            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));
            if(atoi(option1) >= 6 || atoi(option1) < 0)
            {
                return 0;
            }

            if(A4SGUIData->usbhidPtzTime == 0)
            {
                sprintf( msgbuf, "%s", param+strlen(option1)+1);
                gui_ptzControl_byWinID(GUIData, "setzoomin", A4SGUIData->activePTZWinID, msgbuf);
            }
            A4SGUIData->usbhidPtzTime = gettickcount();
        }
        else if ( !strcasecmp( item, "zoomout"))
        {
            char option1[64] = "";
            char msgbuf[64] = "";

            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));
            if(atoi(option1) >= 6 || atoi(option1) < 0)
            {
                return 0;
            }

            if(A4SGUIData->usbhidPtzTime == 0)
            {
                sprintf( msgbuf, "%s", param+strlen(option1)+1);
                gui_ptzControl_byWinID(GUIData, "setzoomout", A4SGUIData->activePTZWinID, msgbuf);
            }
            A4SGUIData->usbhidPtzTime = gettickcount();
        }
        else if ( !strcasecmp( item, "usbhid") || !strcasecmp( item, "setmove") || !strcasecmp( item, "setzoomin")
            || !strcasecmp( item, "setzoomout") || !strcasecmp( item, "setzoomstop"))
        {
            char option1[64] = "";
            char msgbuf[64] = "";

            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));
            if(atoi(option1) >= 6 || atoi(option1) < 0)
            {
                return 0;
            }

            sprintf( msgbuf, "%s", param+strlen(option1)+1);
            gui_ptzControl_byWinID(GUIData, item, atoi(option1), msgbuf);
        }
    }
    else if (!strcasecmp(group, "gui"))
    {
        if ( !strcasecmp( item, "setzoom"))
        {
            char option1[64] = "";
            char option2[64] = "";
            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));
            ava_mtsubs( param, ' ', 2, option2, sizeof( option2));
            if(atoi(option1) >= 6 || atoi(option1) < 0)
            {
                return 0;
            }
            A4SGUIData->ptzZoomValue[atoi(option1)] = atoi(option2);
        }
        else if ( !strcasecmp( item, "activeptz"))
        {
            int activePTZ;
            char option1[64] = "";
            char msgbuf[64] = "";

            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));

            if(atoi(option1) >= GUIData->product->monitorVideos || atoi(option1) < 0)
            {
                return 0;
            }

            activePTZ = gui_getPtzID_byWinID(GUIData, atoi(option1), NULL);
            if(activePTZ >= 0 && A4SGUIData->activePTZWinID != atoi(option1))
            {
                A4SGUIData->activePTZWinID = atoi(option1);

                sprintf( msgbuf, "pc gui activeptz %d", A4SGUIData->activePTZWinID);
                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_GUI,  COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf) + 1 );
                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_KMM,  COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf) + 1 );
            }
        }
        else if (!strcasecmp(item, "interaSourceSwitch"))
        {
            if(strlen(param) > 0)
                gui_interactSource_Process(GUIData, param);
        }
        else if (!strcasecmp(item, "interactSource"))
        {
            char url[64] = "";
            char sbuf[64] = "";
            char option1[64] = "";
            char msgbuf[128] = "";

            strcpy(msgbuf, "pc gui interactSource");
            GetVPValue(GUIData, "interaSource", url, sizeof(url));
            if(strlen(ava_mtsubs( url, '/', 1, sbuf, sizeof( sbuf))) > 0)
            {
                if(strlen(ava_mtsubs( param, ' ', 1, option1, sizeof( option1))) > 0 && !gui_param_exist( sbuf, option1, ','))
                {
                    sprintf(A4SGUIData->interaVideo[0], "%d", GUIData->product->videoWindowInfo.videoWindow[atoi(option1)-1].vpssID);
                }
                else
                {
                    ava_mtsubs( sbuf, ',', 1, option1, sizeof( option1));
                }
                sprintf(msgbuf, "%s %s", msgbuf, option1);
            }
            else
                strcat(msgbuf, " ");
            if(strlen(ava_mtsubs( url, '/', 2, sbuf, sizeof( sbuf))) > 0)
            {
                if(strlen(ava_mtsubs( param, ' ', 2, option1, sizeof( option1))) > 0 && !gui_param_exist( sbuf, option1, ','))
                {
                    sprintf(A4SGUIData->interaVideo[1], "%d", GUIData->product->videoWindowInfo.videoWindow[atoi(option1)-1].vpssID);
                }
                else
                {
                    ava_mtsubs( sbuf, ',', 1, option1, sizeof( option1));
                }
                sprintf(msgbuf, "%s %s", msgbuf, option1);
            }
            else
                strcat(msgbuf, " ");

            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
        else if (!strcasecmp(item, "interaDispMode"))
        {
            char msgbuf[64] = "";
            if(atoi(param) < 0 || atoi(param) > 2 || A4SGUIData->interaDispMode == atoi(param))
                return 0;

            A4SGUIData->interaDispMode = atoi(param);

            if(A4SGUIData->interactRTSPNum > 1)
            {
                if(strlen(A4SGUIData->interaSignal) > 0)
                {
                    int i = ava_cnts(A4SGUIData->interaSignal, ' ');
                    if(A4SGUIData->interaDispMode == 0)
                        sprintf(msgbuf, "modex 1 1+%d %s %s", i, A4SGUIData->interaVideo[7], A4SGUIData->interaSignal);
                    else if(A4SGUIData->interaDispMode == 1)
                        sprintf(msgbuf, "modex 1 MR1+%d %s %s", i, A4SGUIData->interaSignal, A4SGUIData->interaVideo[7]);
                    else if(A4SGUIData->interaDispMode == 2)
                        sprintf(msgbuf, "modex 1 R1+%d %s", i, A4SGUIData->interaSignal);
                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf) + 1 );
                }
                else
                {
                    sprintf(msgbuf, "modex 1 M1 %s", A4SGUIData->interaVideo[7]);
                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf) + 1 );
                }
            }

            sprintf(msgbuf, "pc gui interaDispMode %d", A4SGUIData->interaDispMode);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
        else if (!strcasecmp(item, "interaNetLabelEnable"))
        {
            int option, i, num;
            char name[64] = "";
            char msgbuf[128] = "";

            A4SGUIData->interaNetLabelEnable = atoi(param);

            for(num=1; num<=A4SGUIData->interactRTSPNum; num++)
            {
                option = -1;
                sprintf(name, "label%02d", 5 - num);
                for ( i=0; i< GUIData->osdCount; i++)
                {
                    if (!strcasecmp( GUIData->osd[i].name, name))
                    {
                        option = i;
                        break;
                    }
                }
                if(option >= 0)
                {
                    if(A4SGUIData->interaNetLabelEnable == 1)
                        GUIData->osd[option].enabled = 1;
                    else
                        GUIData->osd[option].enabled = 0;
                    strcpy(GUIData->osd[option].text, "");
                }
            }

            sprintf(msgbuf, "pc gui interaNetLabelEnable %d", A4SGUIData->interaNetLabelEnable);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf, strlen( msgbuf)+1);
        }
        else if (!strcasecmp(item, "RecordVideoInfoEnable"))
        {
            char msgbuf[128] = "";

            A4SGUIData->RecordVideoInfoEnable = atoi(param);
            RecordVideoInfoChange(GUIData);

            sprintf(msgbuf, "pc gui RecordVideoInfoEnable %d", A4SGUIData->RecordVideoInfoEnable);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                            msgbuf, strlen( msgbuf)+1);
        }
        else if (!strcasecmp(item, "interaOutputEnable"))
        {
            char msgbuf[128] = "";
            sprintf(msgbuf, "%s", param);
            SetConfigData(GUIData, "gui", "interaOutputEnable", msgbuf);
        }
    }
    else if (!strcasecmp(group, "INTERA"))
    {
        if (!strcasecmp(item, "interactState"))
        {
            int mode, rtspnum;
            char option1[64] = "";
            char sbuf[128] = "";
            char msgbuf[256] = "";
            if(A4SGUIData->PanelInteraSwitch == 0 && (atoi(param) == 0 || atoi(param) == 1 || atoi(param) == 4) && A4SGUIData->interaMode != atoi(param))
            {
                if(A4SGUIData->interaMode != 0)
                {
                    char name[64] = "";
                    char sbuf2[128] = "";

                    rtspnum = 0;
                    for(i=0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                    {
                        if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "SIP") || !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "RTSP", 4) )
                        {
                            if(!strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].namec, "WIFI", 4)
                                || !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].namec, "HDMIWIFI", 8))
                                continue;

                            rtspnum ++;
                            sprintf(name, "signalurl%02d", i+1);
                            GetConfigData(GUIData, "vrm", name, sbuf, sizeof(sbuf));
                            ava_mtargs( sbuf, "rtspurl", sbuf2, sizeof(sbuf2));
                            if(!strncmp(sbuf2, "sip:", 4) || !strncmp(sbuf2, "h323:", 5))
                                sprintf(msgbuf, "%s rtspurl=%s,rtspstate=sipstop", name, sbuf2);
                            else
                                sprintf(msgbuf, "%s rtspurl=%s,rtspstate=stop", name, sbuf2);
                            ava_mtargs( sbuf, "rtspmode", sbuf2, sizeof(sbuf2));
                            sprintf(msgbuf, "%s,rtspmode=%s", msgbuf, sbuf2);
                            ava_mtargs( sbuf, "rtspspmode", sbuf2, sizeof(sbuf2));
                            sprintf(msgbuf, "%s,rtspspmode=%s", msgbuf, sbuf2);
                            ava_mtargs( sbuf, "rtsptime", sbuf2, sizeof(sbuf2));
                            sprintf(msgbuf, "%s,rtsptime=%s", msgbuf, sbuf2);
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VRM, COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf)+1);
                            if(rtspnum >= A4SGUIData->interactRTSPNum)
                                break;
                        }
                    }
                }

                usleep(300000);

                rtspnum = 0;
                for(i=0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                {
                    if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "SIP") || !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "RTSP", 4) )
                    {
                        if(!strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].namec, "WIFI", 4)
                            || !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].namec, "HDMIWIFI", 8))
                            continue;

                        rtspnum ++;
                        sprintf(option1, "signalurl%02d", i+1);
                        SetConfigData(GUIData, "vrm", option1, "");
                        if(rtspnum >= A4SGUIData->interactRTSPNum)
                            break;
                    }
                }

                for(i=1; i<=3; i++)
                {
                    sprintf(msgbuf, "pc gui interactname%d  ", i);
                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_SYS,  COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf) + 1 );
                }

                strcpy(msgbuf, "stop");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_AT, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);

                strcpy(msgbuf, "clearstatus");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_RTSP, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);

                strcpy(msgbuf, "interasignalcnt 0");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_INTERA, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);

                strcpy(msgbuf, "VGA UNLOCK");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_AT, COMMAND_BY_CHAR,
                                msgbuf, strlen(msgbuf)+1);

                strcpy(msgbuf, "key_response a3off");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                msgbuf, strlen(msgbuf)+1);

                GetConfigData(GUIData, "gui", "interactSource", sbuf, sizeof(sbuf));
                mode = atoi(ava_mtsubs( sbuf, ' ', 1, option1, sizeof( option1)));
                if(mode <= 0)
                    mode = 1;
                else if(mode >= GUIData->product->monitorVideos)
                    mode = GUIData->product->monitorVideos - 1;

                if(A4SGUIData->interaMode == 0)
                {
                    strcpy(msgbuf, "interaswitch 1");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_ARM, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "start 12");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "type none");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                }
                else if(atoi(param) == 0)
                {
                    strcpy(msgbuf, "interaswitch 0");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_ARM, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "mode -1 12");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "stop 12");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "mask 3");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "mode v1");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                }

                A4SGUIData->interaMode = atoi(param);
                if(A4SGUIData->interaMode == 1)
                {
                    strcpy(msgbuf, "vpss 4 34");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    sprintf(msgbuf, "mode v%d 12", mode);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    sprintf(msgbuf, "modex 3 M1 %s", A4SGUIData->interaVideo[4]);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    Gui_MainOsdTarget_Change(GUIData, 0);
                    Gui_InteraOsd_Change(GUIData, 1, -1);
                    a4s_osd_disable(GUIData);
                }
                else if(A4SGUIData->interaMode == 4)
                {
                    sprintf(msgbuf, "vpss 4 %d", (atoi(A4SGUIData->interaVideo[7])<<8)+34);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    sprintf(msgbuf, "mode v%d 14", mode);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    if(A4SGUIData->interaProxySwitch == 2)
                    {
                        sprintf(msgbuf, "modex 8 M1 %s", A4SGUIData->interaVideo[0]);
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                        sprintf(msgbuf, "modex 16 M1 %s", A4SGUIData->interaVideo[1]);
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                        sprintf(msgbuf, "modex 32 M1 %s", A4SGUIData->interaVideo[2]);
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                    }
                    if(A4SGUIData->interactRTSPNum > 1)
                        sprintf(msgbuf, "modex 1 M1 %s", A4SGUIData->interaVideo[7]);
                    else
                        sprintf(msgbuf, "modex 1 1+1 %s %s", A4SGUIData->interaVideo[4], A4SGUIData->interaVideo[7]);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    Gui_MainOsdTarget_Change(GUIData, 1);
                    Gui_InteraOsd_Change(GUIData, 1, -1);
                    a4s_osd_enable(GUIData);
                }
                else if(A4SGUIData->interaMode == 0)
                {
                    strcpy(msgbuf, "vpss 4 34");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    Gui_MainOsdTarget_Change(GUIData, 0);
                    Gui_InteraOsd_Change(GUIData, 0, -1);
                    a4s_osd_enable(GUIData);
                }

                if(A4SGUIData->interaMode == 0)
                    strcpy(msgbuf, "mask 3");
                else if(A4SGUIData->interaAT == 3)
                    strcpy(msgbuf, "mask 0");
                else if(A4SGUIData->interaMode == 1)
                    strcpy(msgbuf, "mask 12");
                else if(A4SGUIData->interaMode == 4)
                    strcpy(msgbuf, "mask 14");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                msgbuf, strlen( msgbuf)+1);

                gui_label_reload(GUIData);
            }
        }
        else if (!strcasecmp(item, "interaLabel"))
        {
            int num;
            char option1[64] = "";

            num = atoi(ava_mtsubs( param, ' ', 1, option1, sizeof( option1)));
            if(num < 1 || num > 3 || num > A4SGUIData->interactRTSPNum)
                return -1;

            int option, i;
            char name[64] = "";

            option = -1;
            sprintf(name, "label%02d", 11 - num);
            for ( i=0; i< GUIData->osdCount; i++)
            {
                if (!strcasecmp( GUIData->osd[i].name, name))
                {
                    option = i;
                    break;
                }
            }
            if(option >= 0)
            {
                if(A4SGUIData->interaMode == 4 || (A4SGUIData->interaProxySwitch == 2 && A4SGUIData->interaMode == 1 && A4SGUIData->rmmaUserID >= 1))
                {
                    GUIData->osd[option].enabled = 1;
                    ava_mtsubs( param, ' ', 2, option1, sizeof( option1));
                    strcpy(GUIData->osd[option].text, option1);
                    if(A4SGUIData->interaProxySwitch == 2 && A4SGUIData->interaMode == 4 && A4SGUIData->rmmaUserID == 0)
                    {
                        char sbuf[256] = "";
                        char msgbuf[256] = "";

                        strcpy(A4SGUIData->rmmaLabel[num], option1);
                        GetConfigData(GUIData, "rtsp", "loginidstun", sbuf, sizeof(sbuf));
                        ava_mtargs( sbuf, "namec", option1, sizeof(option1));
                        strcpy(A4SGUIData->rmmaLabel[0], option1);

                        sprintf(msgbuf, "interaLabel=%s$%s$%s$%s", A4SGUIData->rmmaLabel[0],
                                    A4SGUIData->rmmaLabel[1], A4SGUIData->rmmaLabel[2], A4SGUIData->rmmaLabel[3]);
                        gui_Rmma_Sendmsg(GUIData, 1, msgbuf);
                        gui_Rmma_Sendmsg(GUIData, 2, msgbuf);
                        gui_Rmma_Sendmsg(GUIData, 3, msgbuf);
                    }

                    GUIData->osd[option].BackgroundColor = 0;
                    GUIData->osd[option].fontBorderColor = 0xff0000f8;
                    GUIData->osd[option].fontColor = 0xfff8f8f8;
                    GUIData->osd[option].displayPosX = 20;
                    GUIData->osd[option].displayPosY = 20;
                    GUIData->osd[option].targetVideo = - atoi(A4SGUIData->interaVideo[3+num]) - 1;
                }
                else
                {
                    strcpy(A4SGUIData->rmmaLabel[num], "");
                    strcpy(GUIData->osd[option].text, "");
                }
            }
        }
        else if (!strcasecmp(item, "interaTipsLabel"))
        {
            int num;
            char option1[64] = "";

            num = atoi(ava_mtsubs( param, ' ', 1, option1, sizeof( option1)));
            if(num < 1 || num > 3 || num > A4SGUIData->interactRTSPNum)
                return -1;

            int option, i;
            char name[64] = "";

            option = -1;
            sprintf(name, "label%02d", 8 - num);
            for ( i=0; i< GUIData->osdCount; i++)
            {
                if (!strcasecmp( GUIData->osd[i].name, name))
                {
                    option = i;
                    break;
                }
            }
            if(option >= 0)
            {
                if(A4SGUIData->interaMode == 4)
                {
                    GUIData->osd[option].enabled = 1;
                    ava_mtsubs( param, ' ', 2, option1, sizeof( option1));
                    strcpy(GUIData->osd[option].text, option1);
                    GUIData->osd[option].BackgroundColor = 0;
                    GUIData->osd[option].fontBorderColor = 0xff0000f8;
                    GUIData->osd[option].fontColor = 0xfff8f8f8;
                    GUIData->osd[option].displayPosX = 50;
                    GUIData->osd[option].displayPosY = 100 + num*50;
                    GUIData->osd[option].targetVideo = 2;
                }
                else
                {
                    strcpy(GUIData->osd[option].text, "");
                }
            }
        }
        else if (!strcasecmp(item, "interaNetworkLabel"))
        {
            int num;
            char name[64] = "";
            char option1[128] = "";

            num = atoi(ava_mtsubs( param, ' ', 1, option1, sizeof( option1)));
            if(num < 1 || num > 3 || num > A4SGUIData->interactRTSPNum)
                return -1;

            sprintf(option1, "pc intera interaNetwork %s", param);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                            option1, strlen( option1)+1);
            sprintf(option1, "pc intera interaNetwork%s ", param);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_HTTP, COMMAND_BY_CHAR,
                            option1, strlen( option1)+1);
            if(A4SGUIData->interaNetLabelEnable == 1)
            {
                int option, i;

                option = -1;
                sprintf(name, "label%02d", 5 - num);
                for ( i=0; i< GUIData->osdCount; i++)
                {
                    if (!strcasecmp( GUIData->osd[i].name, name))
                    {
                        option = i;
                        break;
                    }
                }
                if(option >= 0)
                {
                    if(A4SGUIData->interaMode == 0)
                    {
                        strcpy(GUIData->osd[option].text, "");
                    }
                    else
                    {
                        GUIData->osd[option].enabled = 1;
                        ava_mtsubs( param, ' ', 2, option1, sizeof( option1));
                        strcpy(GUIData->osd[option].text, option1);
                        int size = strlen(GUIData->osd[option].oldText);
                        memset(GUIData->osd[option].oldText, ' ', size);
                        GUIData->osd[option].BackgroundColor = 0;
                        GUIData->osd[option].fontBorderColor = 0xff0000f8;
                        GUIData->osd[option].fontColor = 0xfff8f8f8;
                        if(A4SGUIData->interaMode == 4)
                        {
                            GUIData->osd[option].displayPosX = 20;
                            GUIData->osd[option].displayPosY = 60;
                            GUIData->osd[option].targetVideo = - atoi(A4SGUIData->interaVideo[3+num]) - 1;
                        }
                        else if(A4SGUIData->interaMode == 1)
                        {
                            GUIData->osd[option].displayPosX = 50;
                            GUIData->osd[option].displayPosY = 250 + num*50;
                            GUIData->osd[option].targetVideo = 2;
                        }
                    }
                }
            }
        }
        else if (!strcasecmp(item, "interaAT"))
        {
            char msgbuf[64] = "";

            if(!strcasecmp(param, "all"))
                A4SGUIData->interaAT = 0;
            else if(!strcasecmp(param, "teacher"))
                A4SGUIData->interaAT = 1;
            else if(!strcasecmp(param, "student"))
                A4SGUIData->interaAT = 2;
            else if(!strcasecmp(param, "none"))
                A4SGUIData->interaAT = 3;
            else
                A4SGUIData->interaAT = 0;

            if(A4SGUIData->interaMode == 0)
                strcpy(msgbuf, "mask 3");
            else if(A4SGUIData->interaAT == 3)
                strcpy(msgbuf, "mask 0");
            else if(A4SGUIData->interaMode == 1)
                strcpy(msgbuf, "mask 12");
            else if(A4SGUIData->interaMode == 4)
                strcpy(msgbuf, "mask 14");
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                            msgbuf, strlen( msgbuf)+1);

            sprintf(msgbuf, "trackmode %s", param);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_AT, COMMAND_BY_CHAR,
                            msgbuf, strlen( msgbuf)+1);
        }
        else if (!strcasecmp(item, "LoopBack"))
        {
            char option1[64] = "";
            char sbuf[64] = "";
            char msgbuf[128] = "";

            GetConfigData(GUIData, "gui", "interactSource", sbuf, sizeof(sbuf));
            int mode = atoi(ava_mtsubs( sbuf, ' ', 1, option1, sizeof( option1)));
            if(mode <= 0)
                mode = 1;
            else if(mode >= GUIData->product->monitorVideos)
                mode = GUIData->product->monitorVideos - 1;

            if (!strcasecmp(param, "intera") && A4SGUIData->interaLoopBack != 0)
            {
                A4SGUIData->interaLoopBack = 0;
                if(A4SGUIData->interaMode == 4)
                {
                    sprintf(msgbuf, "mode v%d 14", mode);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    if(A4SGUIData->interactRTSPNum > 1)
                    {
                        if(strlen(A4SGUIData->interaSignal) > 0)
                        {
                            int i = ava_cnts(A4SGUIData->interaSignal, ' ');
                            if(A4SGUIData->interaDispMode == 0)
                                sprintf(msgbuf, "modex 1 1+%d %s %s", i, A4SGUIData->interaVideo[7], A4SGUIData->interaSignal);
                            else if(A4SGUIData->interaDispMode == 1)
                                sprintf(msgbuf, "modex 1 MR1+%d %s %s", i, A4SGUIData->interaSignal, A4SGUIData->interaVideo[7]);
                            else if(A4SGUIData->interaDispMode == 2)
                                sprintf(msgbuf, "modex 1 R1+%d %s", i, A4SGUIData->interaSignal);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf) + 1 );
                        }
                        else
                        {
                            sprintf(msgbuf, "modex 1 M1 %s", A4SGUIData->interaVideo[7]);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf) + 1 );
                        }
                    }
                    else if(strlen(A4SGUIData->interaSignal) > 0 && ava_cnts(A4SGUIData->interaSignal, ' ') == 1)
                    {
                        sprintf(msgbuf, "modex 1 1+1 %s %s", A4SGUIData->interaSignal, A4SGUIData->interaVideo[7]);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf)+1);
                    }
                }
            }
            else if (!strcasecmp(param, "remote") && A4SGUIData->interaLoopBack != 1)
            {
                A4SGUIData->interaLoopBack = 1;
                if(A4SGUIData->interaMode == 4)
                {
                    if(strlen(A4SGUIData->interaSignal) > 0)
                    {
                        int i, j;
                        i = ava_cnts(A4SGUIData->interaSignal, ' ');
                        sprintf(msgbuf, "modex 1 M%d %s", i, A4SGUIData->interaSignal);
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                        sprintf(msgbuf, "mode v%d 14", mode);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf)+1);
                        if(i == 1)
                        {
                            for(j=0; j<3; j++)
                            {
                                if(!strcasecmp(A4SGUIData->interaVideo[4+j], A4SGUIData->interaSignal))
                                {
                                    sprintf(msgbuf, "resolution %d 1080P", j+1);
                                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VRM,  COMMAND_BY_CHAR,
                                                    msgbuf,  strlen( msgbuf) + 1 );
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        sprintf(msgbuf, "modex 1 M1 %s", A4SGUIData->interaVideo[4]);
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                        strcpy(msgbuf, "resolution 1 1080P");
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VRM,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                    }
                }
            }
        }
        else if (!strcasecmp(item, "interaconnect"))
        {
            int rtspnum;
            char sbuf[128] = "";
            char name[64] = "";
            char option1[128] = "";
            char msgbuf[128] = "";

            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));

            if(A4SGUIData->RTSPPtzEnable == 1)
            {
                for(i=0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                {
                    if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, option1)
                        && !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].namec, "WIFI", 4))
                    {
                        ava_mtsubs( param, ' ', 2, option1, sizeof( option1));
                        if(!strcasecmp(option1, "connected"))
                        {
                            sprintf(name, "signalurl%02d", i+1);
                            GetConfigData(GUIData, "vrm", name, sbuf, sizeof( sbuf));
                            ava_mtargs( sbuf, "rtspurl", option1, sizeof(option1));
                            ava_mtsubs( option1, '/', 3, sbuf, sizeof(sbuf));
                            ava_mtsubs( sbuf, ':', 1, option1, sizeof(option1));

                            sprintf(msgbuf, "rtspptz %d connect %s", atoi(GUIData->product->videoSignalInfo.videoResource[i].namec+4)-1, option1);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_PTZ,  COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf) + 1 );
                        }
                        else if(!strcasecmp(option1, "disconnected"))
                        {
                            sprintf(msgbuf, "rtspptz %d disconnect", atoi(GUIData->product->videoSignalInfo.videoResource[i].namec+4)-1);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_PTZ,  COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf) + 1 );
                        }
                        break;
                    }
                }
            }

            memset(name, 0, sizeof(name));
            rtspnum = 0;
            for(i=0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
            {
                if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "SIP") || !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "RTSP", 4) )
                {
                    if(!strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].namec, "WIFI", 4))
                        continue;
                    rtspnum ++;
                    if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, option1))
                    {
                        sprintf(name, "interactConnectStatus%d", rtspnum);
                        break;
                    }
                    if(rtspnum >= A4SGUIData->interactRTSPNum)
                        break;
                }
            }
            if(strlen(name) < 5)
                return 0;

            sprintf(msgbuf, "pc %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);

            strcpy(sbuf, A4SGUIData->interaVideo[rtspnum+3]);
            ava_mtsubs( param, ' ', 2, option1, sizeof( option1));
            if(!strcasecmp(option1, "connected"))
            {
                if(strlen(A4SGUIData->interaSignal) > 0)
                {
                    if(gui_param_exist(A4SGUIData->interaSignal, sbuf, ' '))
                        sprintf(A4SGUIData->interaSignal, "%s %s", A4SGUIData->interaSignal, sbuf);
                }
                else
                    strcpy(A4SGUIData->interaSignal, sbuf);
                a4s_module_function(MODULE_RSC,MODULE_RSC,COMMAND_RSC_REPORT_INFO_BYMSG, "act9_stat='1'");

                if(A4SGUIData->interaProxySwitch == 2 && A4SGUIData->interaMode == 4 && A4SGUIData->rmmaUserID == 0)
                {
                    sprintf(msgbuf, "vbmodex=%s", A4SGUIData->rmmaModexMsg);
                    gui_Rmma_Sendmsg(GUIData, rtspnum, msgbuf);
                }
            }
            else if(!strcasecmp(option1, "disconnected"))
            {
                if(strlen(A4SGUIData->interaSignal) > 0)
                {
                    gui_param_delete(A4SGUIData->interaSignal, sbuf, ' ');
                    a4s_module_function(MODULE_RSC,MODULE_RSC,COMMAND_RSC_REPORT_INFO_BYMSG, "act10_stat='1'");
                }
                if(A4SGUIData->interaProxySwitch == 2 && A4SGUIData->rmmaUserID > 0 && A4SGUIData->rmmaUserID < 4)
                {
                    sprintf(msgbuf, "ctrl intera interaLabel %d ", rtspnum);
                    printf("\n\n=====GUI %s=====\n\n", msgbuf);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR, msgbuf, strlen(msgbuf)+1);
                }

                if(strlen(A4SGUIData->interaSignal) == 0 && A4SGUIData->interaMode == 1 && A4SGUIData->rmmaUserID  >= 1)
                {
                    sprintf(msgbuf, "modex 3 M1 %s", A4SGUIData->interaVideo[4]);
                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf) + 1 );
                }
            }
            else
                return 0;

            if(A4SGUIData->interaMode != 0)
            {
                if(strlen(A4SGUIData->interaSignal) > 0)
                    Gui_InteraOsd_Change(GUIData, 0, -1);
                else
                    Gui_InteraOsd_Change(GUIData, 1, -1);
            }

            if(A4SGUIData->DrawPadEnable == 1)
            {
                int i, idx;

                idx = 0;
                if(strlen(A4SGUIData->interaSignal) > 0)
                {
                    for(i=0; i<3; i++)
                    {
                        if(!gui_param_exist(A4SGUIData->interaSignal, A4SGUIData->interaVideo[i+4], ' '))
                            idx |= 0x01 << i;
                    }
                }
                sprintf(msgbuf, "drawpad remotertsp %d", idx);
                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_KMM,  COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf) + 1 );
            }

            if(strlen(A4SGUIData->interaSignal) == 0)
            {
                strcpy(msgbuf, "interastatus 0");
                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_RSC,  COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf) + 1 );
            }
            else if(strlen(A4SGUIData->interaSignal) > 0)
            {
                strcpy(msgbuf, "interastatus 1");
                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_RSC,  COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf) + 1 );
            }

            if(A4SGUIData->PanelInteraSwitch == 1 || A4SGUIData->interaMode == 1)
            {
                if(strlen(A4SGUIData->interaSignal) > 0)
                {
                    if(A4SGUIData->rmmaUserID < 0)
                    {
                        strcpy(msgbuf, "resolution 1 1080P");
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VRM,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                    }
                }
                else if(!strcasecmp(option1, "disconnected"))
                {
                    strcpy(msgbuf, "clearstatus");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_RTSP, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    A4SGUIData->interaMode = 1;
                }
            }
            else if(A4SGUIData->interaMode == 4 && A4SGUIData->interaLoopBack == 1)
            {
                if(strlen(A4SGUIData->interaSignal) > 0)
                {
                    int i = ava_cnts(A4SGUIData->interaSignal, ' ');
                    sprintf(msgbuf, "modex 1 M%d %s", i, A4SGUIData->interaSignal);
                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf) + 1 );
                    if(i == 1)
                    {
                        int j;
                        for(j=0; j<3; j++)
                        {
                            if(!strcasecmp(A4SGUIData->interaVideo[4+j], A4SGUIData->interaSignal))
                            {
                                sprintf(msgbuf, "resolution %d 1080P", j+1);
                                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VRM,  COMMAND_BY_CHAR,
                                                msgbuf,  strlen( msgbuf) + 1 );
                                break;
                            }
                        }
                    }
                }
                else
                {
                    sprintf(msgbuf, "modex 1 M1 %s", A4SGUIData->interaVideo[4]);
                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf) + 1 );
                    sprintf(msgbuf, "resolution 1 1080P");
                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VRM,  COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf) + 1 );
                }
            }
            else if(A4SGUIData->interaMode == 4)
            {
                if(A4SGUIData->interactRTSPNum > 1)
                {
                    if(strlen(A4SGUIData->interaSignal) > 0)
                    {
                        int i = ava_cnts(A4SGUIData->interaSignal, ' ');
                        if(A4SGUIData->interaDispMode == 0)
                            sprintf(msgbuf, "modex 1 1+%d %s %s", i, A4SGUIData->interaVideo[7], A4SGUIData->interaSignal);
                        else if(A4SGUIData->interaDispMode == 1)
                            sprintf(msgbuf, "modex 1 MR1+%d %s %s", i, A4SGUIData->interaSignal, A4SGUIData->interaVideo[7]);
                        else if(A4SGUIData->interaDispMode == 2)
                            sprintf(msgbuf, "modex 1 R1+%d %s", i, A4SGUIData->interaSignal);
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                    }
                    else
                    {
                        sprintf(msgbuf, "modex 1 M1 %s", A4SGUIData->interaVideo[7]);
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf) + 1 );
                    }
                }
                else if(strlen(A4SGUIData->interaSignal) > 0 && ava_cnts(A4SGUIData->interaSignal, ' ') == 1)
                {
                    sprintf(msgbuf, "modex 1 1+1 %s %s", A4SGUIData->interaSignal, A4SGUIData->interaVideo[7]);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                }
            }
        }
        else if (!strcasecmp(item, "regstate"))
        {
            char msgbuf[64] = "";
            if(!strcasecmp(param, "connected"))
                Gui_InteraOsd_Change(GUIData, -1, 1);
            else
                Gui_InteraOsd_Change(GUIData, -1, 0);

            sprintf(msgbuf, "pc %s", command);
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                            msgbuf,  strlen( msgbuf)+1);
        }
    }

    return 0;
}

static int executeGUICommandParamChange( WindowsStruct *windows, WindowStruct * window, GUI_DATA * GUIData, char *command)
{
    A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;

    int controlID, i;
    char * param;
    char group[64] = "";
    char item[64] = "";

    ava_mtsubs(command, ' ', 1, group, sizeof(group));
    ava_mtsubs(command, ' ', 2, item, sizeof( item));
    param = command + strlen(group) + strlen( item) + 2;

    if (!strcasecmp( group, "UI"))
    {
        char attrib[50];
        ava_mtsubs(param, ' ', 1, attrib, sizeof( attrib));
        param += strlen( attrib) + 1;
        AVA_Windows_SetValue( windows, window, item, attrib, param);
    }
    else if (!strcasecmp(group, "setfileinfo"))
    {
        executeUIChange_FileInfo( windows, window, GUIData, command + strlen(group) + 1);
    }
    else if (!strcasecmp( group, "blend"))
    {
        executeUIChange_Vb(windows, window, GUIData, command, group, item, param);
    }
    else if (!strcasecmp( group, "program"))
    {
        executeUIChange_TitileInfo(windows, window, GUIData, command, group, item, param);
    }
    else if (!strcasecmp( group, "osd"))
    {
        executeUIChange_OSD(windows, window, GUIData, command, group, item, param);
    }
    else if (!strcasecmp(group, "cue"))
    {
        if ( !strncasecmp( item, "window", 6))
        {
            int windowID, controlID , i, n;
            listBox_st * list;
            char name[16] = "";
            char sbuf[64] = "";
            char windowName[128] = "";
            char namebuf[128] = "";

            windowID = atoi( item+6 ) - 1;
            sprintf( name, "vs%02d", windowID);
            controlID = AVA_Windows_GetControlID(window, name);
            if ( controlID >= 0)
            {
                char sbuf2[64] = "";
                list = window->control[controlID];
                ava_mtsubs( param, ':', 1, sbuf2, sizeof(sbuf2));
                n = ava_cnts( list->valueTable, '\t');
                for (i=0; i<n; i++)
                {
                    if (!strcmp( sbuf2, ava_mtsubs( list->valueTable, '\t', i+1, sbuf, sizeof(sbuf))))
                    {
                        list->value = i;
                        ava_mtsubs( param, ':', 2, sbuf2, sizeof(sbuf2));
                        if(strlen(sbuf2) > 0 || atoi(sbuf2) > 1)
                            list->value += atoi(sbuf2) -1 ;
                        break;
                    }
                    else if (!strcmp( param, ava_mtsubs( list->valueTable, '\t', i+1, sbuf, sizeof(sbuf))))
                    {
                        list->value = i;
                        ava_mtsubs( param, ':', 2, sbuf2, sizeof(sbuf2));
                        break;
                    }
                }
                AVA_Windows_RedrawControl(windows, window, controlID);

                if(GUIData->currentLanguage == LANGUAGE_ENGLISH)
                {
                    GetVPValue(GUIData, "engwindowName", windowName, sizeof(windowName));
                }
                else if(GUIData->currentLanguage == LANGUAGE_TRIDITIONALCHINESE)
                {
                    GetVPValue(GUIData, "trdwindowName", windowName, sizeof(windowName));
                }
                else
                {
                    GetVPValue(GUIData, "windowName", windowName, sizeof(windowName));
                }
                ava_mtsubs( list->caption, '\t', list->value+1, sbuf, sizeof(sbuf));
                if(strlen(windowName) > 0)
                {
                    ava_mtsubs( windowName, '/', windowID+1, sbuf2, sizeof( sbuf2));
                    if(strlen(sbuf2) > 0)
                        strcpy(sbuf, sbuf2);
                }
                sprintf( name, "bm%02d", windowID);
                controlID = AVA_Windows_GetControlID(window, name);
                if ( controlID >= 0)
                {
                    button_st * bt = window->control[controlID];
                    if(strcmp(bt->caption, sbuf))
                    {
                        strcpy(bt->caption, sbuf);
                        AVA_Windows_RedrawControl(windows, window, controlID);
                    }
                }

                strcpy(namebuf, sbuf);
                if(A4SGUIData->MainArrange[7] > 0)
                {
                    int mode, i;
                    char option1[64] = "";
                    char option2[64] = "";
                    char sbuf2[64] = "";
                    char url[64] = "";
                    char url2[64] = "";
                    button_st *bt;
                    panel_st*pl;

                    GetConfigData(GUIData, "gui", "interactSource", url2, sizeof(url2));
                    GetVPValue(GUIData, "interaSource", url, sizeof(url));
                    if(strlen(ava_mtsubs( url, '/', 1, sbuf, sizeof( sbuf))) > 0)
                    {
                        mode = 0;
                        memset(sbuf2, 0, sizeof(sbuf2));
                        ava_mtsubs( url2, ' ', 1, option2, sizeof( option2));
                        i = 1;
                        while(strlen(ava_mtsubs( sbuf, ',', i, option1, sizeof( option1))) > 0)
                        {
                            if(strlen(option2) > 0 && !strcasecmp(option1, option2))
                                mode = i - 1;
                            i++;
                        }
                        // ava_mtsubs( sbuf, ',', mode + 1, option1, sizeof( option1));
                        if(windowID + 1 == atoi(option1))///2333
                        {/*2333*/
                        233333
                            /*controlID = AVA_Windows_GetControlID(window, "InteratSource1");
                            if(controlID >= 0)
                            {*/
                                bt = window->control[controlID];
                                strcpy(bt->caption, namebuf);
                                controlID = AVA_Windows_GetControlID(window, "panel_8");
                                if(controlID >= 0)
                                {
                                    pl = window->control[controlID];
                                    if(window && pl->basicInfo.visible)
                                        AVA_Windows_RedrawControlByName(windows, window, "InteratSource1");
                                }
                            }
                        }
                    }
                    if(strlen(ava_mtsubs( url, '/', 2, sbuf, sizeof( sbuf))) > 0)
                    {
                        mode = 0;
                        memset(sbuf2, 0, sizeof(sbuf2));
                        ava_mtsubs( url2, ' ', 2, option2, sizeof( option2));
                        i = 1;
                        while(strlen(ava_mtsubs( sbuf, ',', i, option1, sizeof( option1))) > 0)
                        {
                            if(strlen(option2) > 0 && !strcasecmp(option1, option2))
                                mode = i - 1;
                            i++;
                        }
                        ava_mtsubs( sbuf, ',', mode + 1, option1, sizeof( option1));
                        if(windowID + 1 == atoi(option1))
                        {
                            controlID = AVA_Windows_GetControlID(window, "InteratSource2");
                            if(controlID >= 0)
                            {
                                bt = window->control[controlID];
                                strcpy(bt->caption, namebuf);
                                controlID = AVA_Windows_GetControlID(window, "panel_8");
                                if(controlID >= 0)
                                {
                                    pl = window->control[controlID];
                                    if(window && pl->basicInfo.visible)
                                        AVA_Windows_RedrawControlByName(windows, window, "InteratSource2");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (!strcasecmp( group, "ftpc"))
    {
        text_st *tx = NULL;
        controlID = AVA_Windows_GetControlID(window, "DataInfo7");
        if(controlID >= 0)
        {
            tx = window->control[controlID];
        }
        else
        {
            return -1;
        }
        
        if( strstr(param, "ParamError") )
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_3, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }
        else if( strstr(param, "Process"))
        {
            int i=0;
            char str[16] = "";
            char *p = strstr(param, "Process=");
            p += 8;

            while(*p != ' ')
            {
                str[i] = *(p++);
                i++;
            }
            str[i] = '\0';

            p = strstr(param, "Filecount=");
            p += 10;
            sprintf(tx->caption, "%s (1/%s)", str, p);
            AVA_Windows_RedrawControl(windows, window, controlID);

        }

        else if( strstr(param, "NoMission") )
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_1, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }
        else if( strstr(param, "Hostname&port") )
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_5, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }
        else if( strstr(param, "User") )
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_6, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }
        else if( strstr(param, "Connect") )
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_7, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }
        else if( strstr(param, "Login") )
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_8, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }
        else if( strstr(param, "Pass") )
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_9, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }

        if( strstr(param, "Done"))
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_4, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }

        if( strstr(param, "Failed"))
        {
            getLanguageString(MAINWINDOW_PROGRAMINFO_DATAINFO7_10, GUIData->currentLanguage, tx->caption);
            AVA_Windows_RedrawControl(windows, window, controlID);
        }

    }
    else if (!strcasecmp( group, "arm"))
    {
        if ( !strcasecmp( item, "phantom"))
        {
            SetDataToUI(window, "phantom48vswitch", param);
            AVA_Windows_RedrawControlByName(windows, window, "phantom48vswitch");
        }
        else if ( !strcasecmp( item, "advmix"))
        {
            SetDataToUI(window, "interaLineinswitch", param);
            AVA_Windows_RedrawControlByName(windows, window, "interaLineinswitch");
        }
        else if ( !strcasecmp( item, "volume"))
        {
            char sbuf[64] = "";
            char path[64] = "";
            char option1[64] = "";
            valueBar_st *vb;

            AVA_GUI_LockScreen( &GUIData->GUIHandle, 1);
            usleep(100000);

            for(i=1; i<=4; i++)
            {
                sprintf(path, "volumename%d", i+1);
                GetDataFromUI(window, path, sbuf, sizeof(sbuf));
                ava_mtargs( param, sbuf, option1, sizeof(option1));
                sprintf(path, "volumeval%d", i+1);
                controlID = AVA_Windows_GetControlID(window, path);
                if(controlID >= 0)
                {
                    vb = window->control[controlID];
                    if(atoi(option1) >= 256)
                    {
                        vb->currentValue = atoi(option1) & 0xff;
                        vb->basicInfo.enabled = 0;
                        strcpy(sbuf, "1");
                    }
                    else if(atoi(option1) >= 0)
                    {
                        vb->currentValue = atoi(option1) & 0xff;
                        vb->basicInfo.enabled = 1;
                        strcpy(sbuf, "0");
                    }
                    else
                    {
                        vb->currentValue = 0;
                        vb->basicInfo.enabled = 1;
                        strcpy(sbuf, "0");
                    }
                    AVA_Windows_RedrawControl(windows, window, controlID);
                    sprintf(path, "volumeswitch%d", i+1);
                    SetDataToUI(window, path, sbuf);
                    AVA_Windows_RedrawControlByName(windows, window, path);
                }
            }
            AVA_GUI_LockScreen( &GUIData->GUIHandle, 0);
        }
        else if(!strcasecmp(item, "avsyncResult"))
        {
            SendCommandToWindow(windows, &adjustVideoAudio, CONTROL_WINDOW_OTHER, param);
        }
    }
    else if (!strcasecmp( group, "ab"))
    {
        if ( !strcasecmp( item, "volume"))
        {
            char sbuf[64] = "";
            char option1[64] = "";
            valueBar_st *vb;

            GetDataFromUI(window, "volumename1", sbuf, sizeof(sbuf));
            ava_mtargs( param, sbuf, option1, sizeof(option1));
            controlID = AVA_Windows_GetControlID(window, "volumeval1");
            if(controlID >= 0)
            {
                vb = window->control[controlID];
                if(atoi(option1) >= 256)
                {
                    vb->currentValue = atoi(option1) & 0xff;
                    vb->basicInfo.enabled = 0;
                    strcpy(sbuf, "1");
                }
                else if(atoi(option1) >= 0)
                {
                    vb->currentValue = atoi(option1) & 0xff;
                    vb->basicInfo.enabled = 1;
                    strcpy(sbuf, "0");
                }
                else
                {
                    vb->currentValue = 0;
                    vb->basicInfo.enabled = 1;
                    strcpy(sbuf, "0");
                }
                AVA_Windows_RedrawControl(windows, window, controlID);
                SetDataToUI(window, "volumeswitch1", sbuf);
                AVA_Windows_RedrawControlByName(windows, window, "volumeswitch1");
            }
        }
        else if ( !strcasecmp( item, "hdmilooplocal"))
        {
            char sbuf[64] = "";
            ava_mtsubs( param, ' ', 1, sbuf, sizeof( sbuf));
            SetDataToUI(window, "hdmilooplocal1switch", sbuf);
            AVA_Windows_RedrawControlByName(windows, window, "hdmilooplocal1switch");
            ava_mtsubs( param, ' ', 2, sbuf, sizeof( sbuf));
            SetDataToUI(window, "hdmilooplocal2switch", sbuf);
            AVA_Windows_RedrawControlByName(windows, window, "hdmilooplocal2switch");
        }
    }
    else if (!strcasecmp( group, "tmi"))
    {
        if ( !strcasecmp( item, "osdpptdetect"))
        {
            if ( !strcasecmp( param, "start"))
            {
                controlID = AVA_Windows_GetControlID(window, "subtitleDefaultText");
                if(controlID >= 0)
                {
                    listBox_st *lb = window->control[controlID];
                    lb->basicInfo.enabled = 0;
                }
                controlID = AVA_Windows_GetControlID(window, "inputSubtitle");
                if(controlID >= 0)
                {
                    sle_st *sle = window->control[controlID];
                    sle->basicInfo.enabled = 0;
                }
                SetDataToUI(window, "SubtitleSelect", "ppt");
            }
            else
            {
                controlID = AVA_Windows_GetControlID(window, "subtitleDefaultText");
                if(controlID >= 0)
                {
                    listBox_st *lb = window->control[controlID];
                    lb->basicInfo.enabled = 1;
                }
                controlID = AVA_Windows_GetControlID(window, "inputSubtitle");
                if(controlID >= 0)
                {
                    sle_st *sle = window->control[controlID];
                    sle->basicInfo.enabled = 1;
                }
                SetDataToUI(window, "SubtitleSelect", "word");
            }

            controlID = AVA_Windows_GetControlID( window, "panel_3");
            if (controlID >= 0)
            {
                panel_st *pl = window->control[controlID];
                if(pl->basicInfo.visible == 1 && pl->basicInfo.enabled == 1 )
                {
                    AVA_GUI_LockScreen( &GUIData->GUIHandle, 1);
                    usleep(100000);

                    AVA_Windows_RedrawControlByName(windows, window, "subtitleDefaultText");
                    AVA_Windows_RedrawControlByName(windows, window, "SubtitleSelect");
                    AVA_Windows_RedrawControlByName(windows, window, "inputSubtitle");
                    AVA_GUI_LockScreen( &GUIData->GUIHandle, 0);
                }
            }
        }
    }
    else if (!strcasecmp(group, "at"))
    {
        if ( !strcasecmp( item, "autotrack"))
        {
            if(!strcasecmp(param, "start"))
            {
                autotrackchange( windows,  window, 1);
            }
            else if(!strcasecmp(param, "stop"))
            {
                autotrackchange( windows,  window, 2);
            }
            else if(!strcasecmp(param, "onlytrack"))
            {
                autotrackchange( windows,  window, 3);
            }
        }
    }
    else if (!strcasecmp(group, "rec"))
    {
        if ( !strcasecmp( item, "info"))
        {
            char sbuf[64] = "";
            A4SGUIData->recBaseLength = atoi( ava_mtsubs( param, ' ', 1, sbuf, sizeof( sbuf)));
            A4SGUIData->recStartTime = atoi( ava_mtsubs( param, ' ', 2, sbuf, sizeof( sbuf)));
            A4SGUIData->recButtonLastSwitchTime = 0;
            if(A4SGUIData->recBaseLength == 0 && A4SGUIData->recStartTime == -1)
            {
                char path[64] = "";
                text_st *tx;

                strcpy( sbuf, "00:00:00");
                for(i=1; i<=9; i++)
                {
                    sprintf(path, "recTime%d", i);
                    controlID = AVA_Windows_GetControlID( window, path);
                    if (controlID >= 0)
                    {
                        tx = window->control[controlID];
                        if(strncasecmp(sbuf+i-1, tx->caption, 1))
                        {
                            strncpy( tx->caption, sbuf+i-1, 1);
                            if(window)
                                AVA_Windows_RedrawControl( windows, window, controlID);
                        }
                    }
                }
                recchange(windows, window, 3);
            }
            else if(A4SGUIData->recBaseLength > 0 && A4SGUIData->recStartTime == -1)
            {
                recchange(windows, window, 2);
            }
            else if(A4SGUIData->recStartTime > 0)
            {
                recchange(windows, window, 1);
            }
            strcpy(sbuf, "ctrl rec info");
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                            sbuf,  strlen( sbuf)+1);

            controlID = AVA_Windows_GetControlID(window, "phantom48vswitch");
            if(controlID >= 0)
            {
                checkBox_st *cb = window->control[controlID];
                if(A4SGUIData->recBaseLength == 0 && A4SGUIData->recStartTime == -1)
                    cb->basicInfo.enabled = 1;
                else
                    cb->basicInfo.enabled = 0;

                AVA_Windows_RedrawControl(windows, window, controlID);
            }
        }
    }
    else if (!strcasecmp(group, "rtsp"))
    {
        if ( !strcasecmp( item, "info"))
        {
            char sbuf[64] = "";
            A4SGUIData->rtspStartTime = atoi( ava_mtsubs( param, ' ', 1, sbuf, sizeof( sbuf)));
            A4SGUIData->rtspButtonLastSwitchTime = 0;
            if(A4SGUIData->rtspStartTime == -1)
            {
                rtspchange(windows, window, 0);
            }
            else if(A4SGUIData->rtspStartTime > 0)
            {
                rtspchange(windows, window, 1);
            }
            strcpy(sbuf, "ctrl rtsp info");
            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                            sbuf,  strlen( sbuf)+1);
        }
        else if ( !strncasecmp( item, "rtmpstate", 9))
        {
            char sbuf[128] = "";
            sprintf(sbuf, "%d %s %s", RTMPSETUP_RTMPSTATE, item+9, param);
            SendCommandToWindow(GUIData->windows, &RTMPSetup, CONTROL_WINDOW_OTHER, sbuf);
        }
    }
    else if (!strcasecmp(group, "rsc"))
    {
        if ( !strcasecmp( item, "ProxySwitch"))
        {
            if(A4SGUIData->interaProxySwitch != atoi(param))
            {
                A4SGUIData->interaProxySwitch = atoi(param);
                printf("=======interaProxySwitch=====%d=========\n", A4SGUIData->interaProxySwitch);

                char sbuf[64] = "";
                GetConfigData(GUIData, "intera", "interasignalcnt", sbuf, sizeof(sbuf));

                if(A4SGUIData->interactRTSPNum >= 1 && atoi(sbuf) >= 1)
                {
                    controlID = AVA_Windows_GetControlID( window, "interactionStop1");
                    if(controlID >= 0)
                        interactionConnect(windows,  window, controlID, 0, NULL, 0);
                }
                if(A4SGUIData->interactRTSPNum >= 2 && atoi(sbuf) >= 2)
                {
                    controlID = AVA_Windows_GetControlID( window, "interactionStop2");
                    if(controlID >= 0)
                        interactionConnect(windows,  window, controlID, 0, NULL, 0);
                }
                if(A4SGUIData->interactRTSPNum >= 3 && atoi(sbuf) >= 3)
                {
                    controlID = AVA_Windows_GetControlID( window, "interactionStop3");
                    if(controlID >= 0)
                        interactionConnect(windows,  window, controlID, 0, NULL, 0);
                }
            }
        }
    }
    else if (!strcasecmp(group, "HARDDISKFORMAT"))
    {
        char option1[64] = "";
        ava_mtsubs( param, ' ', 1, option1, sizeof( option1));
        if (!strcasecmp(item,"RESULT"))
        {
            char sbuf[128] = "";
            sprintf(sbuf, "%d %s", HDDFORMAT_RESULT, option1);
            SendCommandToWindow(GUIData->windows, &sys_harddiskmanage, CONTROL_WINDOW_OTHER, sbuf);
        }
        else if (!strcasecmp(item,"PERCENT"))
        {
            char sbuf[128] = "";
            sprintf(sbuf, "%d %s", HDDFORMAT_PCT, option1);
            SendCommandToWindow(GUIData->windows, &sys_harddiskmanage, CONTROL_WINDOW_OTHER, sbuf);
        }
    }
    else if (!strcasecmp(group, "system"))
    {
        if (!strcasecmp(item, "status"))
        {
            char name[128] = "";
            char sbuf[128] = "";
            ava_mtsubs( command, ' ', 3, name, sizeof( name));
            if(!strcasecmp(name, "SHUTDOWN"))
                getLanguageString( SYS_SHUTDOWN_TIPS1, GUIData->currentLanguage, sbuf);
            else if(!strcasecmp(name, "SLEEP"))
                getLanguageString( SYS_SHUTDOWN_TIPS2, GUIData->currentLanguage, sbuf);
            else if(!strcasecmp(name, "REBOOT"))
                getLanguageString( SYS_SHUTDOWN_TIPS3, GUIData->currentLanguage, sbuf);
            else if(!strcasecmp(name, "SHUTDOWN2"))
                getLanguageString( SYS_SHUTDOWN_TIPS4, GUIData->currentLanguage, sbuf);
            else if(!strcasecmp(name, "RECOVERY"))
                getLanguageString( SYS_RECOVERY_TIPS1, GUIData->currentLanguage, sbuf);
            else if(!strcasecmp(name, "UPDATE"))
                getLanguageString( UPDATE_UPDATESYSTEM_TIPS2, GUIData->currentLanguage, sbuf);
            else
                return 0;

            gui_Tips_display(GUIData, TIPS_LOCK, sbuf);
        }
    }
    else if (!strcasecmp(group, "network"))
    {
        if (!strcasecmp(item, "set"))
        {
            char option1[128] = "";
            char sbuf[128] = "";
            char sbuf2[128] = "";

            ava_mtsubs( command, ' ', 3, option1, sizeof( option1));
            if(!strcasecmp(option1, "fail"))
            {
                ava_mtsubs( command, ' ', 4, option1, sizeof( option1));
                getLanguageString( NETWORKSETUP_TIPS1, GUIData->currentLanguage, sbuf);
                sprintf(sbuf2, "%s %s", option1, sbuf);
                gui_Tips_display(GUIData, TIPS_LOCK, sbuf2);
            }
        }
        else if (!strcasecmp(item, "wifidev"))
        {
            char option1[128] = "";
            char sbuf[128] = "";

            ava_mtsubs( command, ' ', 3, option1, sizeof( option1));
            if(!strcasecmp(option1, "connected"))
                A4SGUIData->WifiState = 1;
            else if(!strcasecmp(option1, "disconnected"))
                A4SGUIData->WifiState = 0;

            sprintf(sbuf, "%d", REFRESH_WIFIDEV);
            SendCommandToWindow(GUIData->windows, &NetworkSetup, CONTROL_WINDOW_OTHER, sbuf);
            SendCommandToWindow(GUIData->windows, &wifiSetup, CONTROL_WINDOW_OTHER, sbuf);
            SendCommandToWindow(GUIData->windows, GUIData->mainWindow, CONTROL_WINDOW_OTHER, sbuf);
        }
    }
    else if (!strcasecmp(group, "UPDATE"))
    {
        char name[64] = "";
        char msg[64] = "";
        ava_mtsubs( param, ' ', 1, name, sizeof( name));
        ava_mtsubs( param, ' ', 2, msg, sizeof( msg));

        if(!strcasecmp(name, "FLI32626"))
        {
            strcpy(name, "VP");
        }
        else if(!strcasecmp(name, "CPLD"))
        {
            strcpy(name, "FPGA3");
        }
        else if(!strcasecmp(name, "MCU"))
        {
            strcpy(name, "MC");
        }
        else if(!strcasecmp(name, "MS182"))
        {
            strcpy(name, "ISP1");
        }
        if (!strcasecmp(item, "RESULT"))
        {
            char sbuf[128] = "";
            if(A4SGUIData->UpdateType == 0)
            {
                sprintf(sbuf, "%d %s %s", UPDATE_RETURN, name, msg);
                SendCommandToWindow(GUIData->windows, &Update, CONTROL_WINDOW_OTHER, sbuf);
            }
            else if(A4SGUIData->UpdateType == 1)
            {
                sprintf(sbuf, "update %s\t%s", name, msg);
                gui_Tips_display(GUIData, TIPS_REFRESH, sbuf);
            }
        }
        else if (!strcasecmp(item, "PERCENT"))
        {
            char sbuf[128] = "";
            if(A4SGUIData->UpdateType == 0)
            {
                sprintf(sbuf, "%d %s %s", UPDATE_PCT, name, msg);
                SendCommandToWindow(GUIData->windows, &Update, CONTROL_WINDOW_OTHER, sbuf);
            }
            else if(A4SGUIData->UpdateType == 1)
            {
                sprintf(sbuf, "update %s\t%s", name, msg);
                gui_Tips_display(GUIData, TIPS_REFRESH, sbuf);
            }
        }
    }
    else if (!strcasecmp(group, "updateServer"))
    {
        if (!strcasecmp(item, "test"))
        {
            char sbuf[128] = "";
            sprintf(sbuf, "%d %s", NETSERVER_RETURN, param);
            SendCommandToWindow(GUIData->windows, &sys_versionInfo, CONTROL_WINDOW_OTHER, sbuf);
        }
    }
    else if (!strcasecmp(group, "autoUpdate"))
    {
        if (!strcasecmp(item, "RESULT"))
        {
            char sbuf[128] = "";
            if (!strncasecmp(param, "FAILURE", 7))
            {
                sprintf(sbuf, "update %s", param);
                gui_Tips_display(GUIData, TIPS_UNLOCK, sbuf);
            }
            else if (!strncasecmp(param, "SUCCESS", 7))
            {
                sprintf(sbuf, "update %s", param);
                gui_Tips_display(GUIData, TIPS_REFRESH, sbuf);
            }
        }
    }
    else if (!strcasecmp(group,"UDPTEST"))
    {
        char sbuf[128] = "";
        char option1[64] = "";
        char msg[64] = "";
        ava_mtsubs( param, ' ', 1, option1, sizeof( option1));
        strcpy(msg, param+strlen(option1)+1);
        if (!strcasecmp(option1, "RESULT") && !strcasecmp(item, "SCAN"))
        {
            sprintf(sbuf, "%d %s", NETWORKDETECT_SCANRESULT, msg);
            SendCommandToWindow(GUIData->windows, &networkDetect, CONTROL_WINDOW_OTHER, sbuf);
        }
        else if (!strcasecmp(option1, "RESULT") && !strcasecmp(item, "ONE"))
        {
            sprintf(sbuf, "%d %s", NETWORKDETECT_ONERESULT, msg);
            SendCommandToWindow(GUIData->windows, &networkDetect, CONTROL_WINDOW_OTHER, sbuf);
        }
    }
    else if (!strcasecmp(group, "vrm"))
    {
        if (!strncasecmp(item, "signalurl", 9))
        {
            char name[64] = "";
            char sbuf[256] = "";
            char sbuf2[64] = "";
            strcpy(name, GUIData->product->videoSignalInfo.videoResource[atoi(item+9)-1].name);
            if(!strcasecmp(name, "SIP") || !strncasecmp(name, "RTSP", 4) || !strncasecmp(name, "H323", 4))
            {
                int rtspnum;
                rtspnum = 0;
                for(i=0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                {
                    if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "SIP")
                        || !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "RTSP", 4)
                        || !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "H323", 4) )
                    {
                        rtspnum ++;
                        if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, name))
                        {
                            if(rtspnum >= 1 && rtspnum <= 3)
                            {
                                ava_mtargs( param, "rtspspmode", sbuf2, sizeof(sbuf2));
                                ava_mtargs( param, "rtspurl", sbuf, sizeof(sbuf));
                                sprintf(name, "interactionurl%d", rtspnum);
                                SetDataToUI(window, name, "");

                                if(atoi(sbuf2) == 1)
                                {
                                    if(!strncmp(sbuf, "rtsp:                                    {
                                        ava_mtsubs( sbuf, '/', 3, sbuf2, sizeof(sbuf2));
                                        ava_mtsubs( sbuf2, ':', 1, sbuf, sizeof(sbuf));
                                        SetDataToUI(window, name, sbuf);
                                        sprintf(name, "interactiontype%d", rtspnum);
                                        SetDataToUI(window, name, "AVA");
                                    }
                                }
                                else if(!strncmp(sbuf, "rtsp:                                {
                                    ava_mtsubs( sbuf, '/', 3, sbuf2, sizeof(sbuf2));
                                    ava_mtsubs( sbuf2, ':', 1, sbuf, sizeof(sbuf));
                                    SetDataToUI(window, name, sbuf);
                                    sprintf(name, "interactiontype%d", rtspnum);
                                    SetDataToUI(window, name, "RTSP");
                                }
                                else if(!strncmp(sbuf, "sip:", 4))
                                {
                                    ava_mtsubs( sbuf, '/', 1, sbuf2, sizeof(sbuf2));
                                    ava_mtsubs( sbuf2, ':', 2, sbuf, sizeof(sbuf));
                                    SetDataToUI(window, name, sbuf);
                                    sprintf(name, "interactiontype%d", rtspnum);
                                    SetDataToUI(window, name, "SIP");
                                }
                                else if(!strncmp(sbuf, "h323:", 4))
                                {
                                    ava_mtsubs( sbuf, '/', 1, sbuf2, sizeof(sbuf2));
                                    ava_mtsubs( sbuf2, ':', 2, sbuf, sizeof(sbuf));
                                    SetDataToUI(window, name, sbuf);
                                    sprintf(name, "interactiontype%d", rtspnum);
                                    SetDataToUI(window, name, "H323");
                                }
                                else
                                    return 0;
                            }
                            break;
                        }
                        if(rtspnum >= A4SGUIData->interactRTSPNum)
                            break;
                    }
                }
            }
        }
    }
    else if (!strcasecmp(group, "gui"))
    {
        if (!strncasecmp(item, "interactname", 12))
        {
            int rtspnum;
            char name[64] = "";
            rtspnum = atoi(item+12);

            sprintf(name, "interactionName%d", rtspnum);
            SetDataToUI(window, name, param);

            controlID = AVA_Windows_GetControlID(window, "panel_8");
            if(controlID >= 0)
            {
                panel_st *pl = window->control[controlID];
                if(pl->basicInfo.visible == 1)
                {
                    sprintf(name, "interactionName%d", rtspnum);
                    AVA_Windows_RedrawControlByName(windows, window, name);
                }
            }

            if(A4SGUIData->PanelInteraSwitch == 1)
            {
                controlID = AVA_Windows_GetControlID(window, "panel_interaction2");
                if(controlID >= 0)
                {
                    panel_st *pl = window->control[controlID];
                    if(pl->basicInfo.visible == 1)
                    {
                        sprintf(name, "interactionName%d", rtspnum);
                        AVA_Windows_RedrawControlByName(windows, window, name);
                    }
                }
            }
        }
        else if (!strcasecmp(item, "interaNetLabelEnable"))
        {
            if(atoi(param) == 0 || atoi(param) == 1)
            {
                SetDataToUI(window, "LossRateSwitch", param);
                if(window)
                    AVA_Windows_RedrawControlByName(windows, window, "LossRateSwitch");
            }
        }
        else if (!strcasecmp(item, "interactSource"))
        {
            char sbuf[64] = "";
            char option1[64] = "";
            button_st *bt;
            panel_st *pl;

            if(strlen(ava_mtsubs( param, ' ', 1, sbuf, sizeof( sbuf))) > 0)
            {
                if(A4SGUIData->interaMode != 0)
                {
                    if(atoi(sbuf) - 1 == 0)
                    {
                        gui_interaction_WIFI_Enable(windows, window, 0, 1);
                        gui_interaction_WIFI_Enable(windows, window, 2, 0);
                    }
                    else
                    {
                        gui_interaction_WIFI_Enable(windows, window, 0, 0);
                        gui_interaction_WIFI_Enable(windows, window, 2, 1);
                    }
                }
                else
                {
                    gui_interaction_WIFI_Enable(windows, window, 0, 1);
                    gui_interaction_WIFI_Enable(windows, window, 2, 1);
                }
                GetWindowName(GUIData, atoi(sbuf), option1);
                controlID = AVA_Windows_GetControlID(window, "InteratSource1");
                if(controlID >= 0)
                {
                    bt = window->control[controlID];
                    strcpy(bt->caption, option1);
                    controlID = AVA_Windows_GetControlID(window, "panel_8");
                    if(controlID >= 0)
                    {
                        pl = window->control[controlID];
                        if(window && pl->basicInfo.visible)
                            AVA_Windows_RedrawControlByName(windows, window, "InteratSource1");
                    }
                }
            }
            if(strlen(ava_mtsubs( param, ' ', 2, sbuf, sizeof( sbuf))) > 0)
            {
                if(A4SGUIData->interaMode != 0)
                {
                    if(atoi(sbuf) - 1 == 1)
                    {
                        gui_interaction_WIFI_Enable(windows, window, 1, 1);
                        gui_interaction_WIFI_Enable(windows, window, 3, 0);
                    }
                    else
                    {
                        gui_interaction_WIFI_Enable(windows, window, 1, 0);
                        gui_interaction_WIFI_Enable(windows, window, 3, 1);
                    }
                    gui_interaction_WIFI_Enable(windows, window, 4, 0);
                }
                else
                {
                    gui_interaction_WIFI_Enable(windows, window, 1, 1);
                    gui_interaction_WIFI_Enable(windows, window, 3, 1);
                    gui_interaction_WIFI_Enable(windows, window, 4, 1);
                }
                GetWindowName(GUIData, atoi(sbuf), option1);
                controlID = AVA_Windows_GetControlID(window, "InteratSource2");
                if(controlID >= 0)
                {
                    bt = window->control[controlID];
                    strcpy(bt->caption, option1);
                    controlID = AVA_Windows_GetControlID(window, "panel_8");
                    if(controlID >= 0)
                    {
                        pl = window->control[controlID];
                        if(window && pl->basicInfo.visible)
                            AVA_Windows_RedrawControlByName(windows, window, "InteratSource2");
                    }
                }
            }
        }
        else if ( !strcasecmp( item, "activeptz"))
        {
            char option1[64] = "";

            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));

            if(atoi(option1) >= 6 || atoi(option1) >= GUIData->product->monitorVideos || atoi(option1) < 0)
            {
                return 0;
            }

            gui_setActivePtz(windows, window, atoi(option1));
        }
        else if ( !strcasecmp( item, "tmp"))
        {
            char sbuf[128] = "";
            char sbuf2[128] = "";
            text_st *tx;

            if(!strcasecmp(param, "safe"))
            {
                controlID = AVA_Windows_GetControlID( window, "CpuTempture");
                if (controlID >= 0)
                {
                    tx = window->control[controlID];
                    tx->basicInfo.frame.backgroundColor = WINDOWS_BGCOLOR;
                }
                getLanguageString( MAINWINDOW_TMP_TIPS3, GUIData->currentLanguage, sbuf);
                gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
            }
            else if(!strcasecmp(param, "warm"))
            {
                char name[64] = "";
                panel_st *pl;
                button_st *bt;

                controlID = AVA_Windows_GetControlID( window, "CpuTempture");
                if (controlID >= 0)
                {
                    tx = window->control[controlID];
                    tx->basicInfo.frame.backgroundColor = 0xffff0000;
                }
                for(i=1; i<10; i++)
                {
                    sprintf(name, "setupbutton_%d", i);
                    controlID = AVA_Windows_GetControlID(window, name);
                    if(controlID >= 0)
                    {
                        bt = window->control[controlID];
                        strcpy(name, bt->caption);
                        ava_mtsubs( name, '_', 1, sbuf, sizeof( sbuf));
                        ava_mtsubs( name, '_', 2, sbuf2, sizeof( sbuf2));
                        if(i == 9)
                            sprintf(bt->caption, "%s_%s_3", sbuf,  sbuf2);
                        else
                            sprintf(bt->caption, "%s_%s", sbuf,  sbuf2);

                    }
                }
                for(i=1; i<10; i++)
                {
                    sprintf(name, "panel_%d", i);
                    controlID = AVA_Windows_GetControlID(window, name);
                    if(controlID >= 0)
                    {
                        pl = window->control[controlID];
                        if(pl->basicInfo.visible == 1 && pl->basicInfo.enabled == 1)
                        {
                            if(i == 9)
                                break;
                            pl->basicInfo.visible = pl->basicInfo.enabled = 0;
                            sprintf(name, "setupbutton_%d", i);
                            AVA_Windows_RedrawControlByName(windows, window, name);
                            AVA_Windows_RedrawControlByName(windows, window, "setupbutton_9");

                            controlID = AVA_Windows_GetControlID(window, "panel_9");
                            if(controlID >= 0)
                            {
                                pl = window->control[controlID];
                                pl->basicInfo.visible = pl->basicInfo.enabled = 1;
                                AVA_Windows_RedrawControl(windows, window, controlID);
                            }
                            break;
                        }
                    }
                }
                getLanguageString( MAINWINDOW_TMP_TIPS2, GUIData->currentLanguage, sbuf);
                gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
            }
            else if(!strcasecmp(param, "shutdown"))
            {
                getLanguageString( MAINWINDOW_TMP_TIPS1, GUIData->currentLanguage, sbuf);
                gui_Tips_display(GUIData, TIPS_LOCK, sbuf);
            }
        }
        else if(!strcasecmp(item, "KeyingEnable"))
        {
            if(A4SGUIData->KeyingEnable != 1)
                return 0;
            int enabled;
            char sbuf[128] = "";
            char option1[64] = "";

            ava_mtsubs(param, ' ', 1, option1, sizeof(option1));
            sprintf( sbuf, "keyingvideo%d", atoi(option1) + 1);
            controlID = AVA_Windows_GetControlID(window, sbuf);
            if(controlID >= 0)
            {
                checkBox_st * cb = window->control[controlID];
                ava_mtsubs(param, ' ', 2, option1, sizeof(option1));
                if(!strcasecmp(option1, "enable"))
                    enabled = 1;
                else
                    enabled = 0;
                if(cb->value != enabled)
                {
                    cb->value = enabled;
                    AVA_Windows_RedrawControl(windows, window, controlID);
                }
                if(cb->value == 1)
                {
                    sprintf( sbuf, "keyingvideo%d", 2-cb->param);
                    controlID = AVA_Windows_GetControlID(window, sbuf);
                    if(controlID >= 0)
                    {
                        cb = window->control[controlID];
                        if(cb->value == 1)
                        {
                            cb->value = 0;
                            AVA_Windows_RedrawControl(windows, window, controlID);
                        }
                    }
                }
            }
        }
        else if(!strcasecmp(item, "KeyingHDMI3Vo"))
        {
            if(A4SGUIData->KeyingEnable != 1)
                return 0;
            char sbuf[128] = "";
            char option1[64] = "";

            ava_mtsubs(param, ' ', 1, option1, sizeof(option1));             if(atoi(option1) >= 0 && atoi(option1) <= 2)
            {
                SetDataToUI(window, "KeyingHDMI3VO", option1);
                AVA_Windows_RedrawControlByName(windows, window, "KeyingHDMI3VO");

            }

            if(atoi(option1) >= 1 && atoi(option1) <= 2)            {
                if(A4SGUIData->DrawPadEnable == 1)
                {
                                        if(A4SGUIData->drawpad.info.winopened == 0)
                    {
                                                strcpy(sbuf, "drawpad open");
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        sbuf,  strlen( sbuf)+1);

                    }

                                        char path[128] = "";
                    DIR * dir;
                    struct dirent * ptr;
                    GetVPValue(GUIData, "updatepath", sbuf, sizeof(sbuf)-1);
                    if(strlen(sbuf)<=0)
                        return -1;

                    snprintf(path, sizeof(path)-1, "%sdrawdata/", sbuf);

                    dir = opendir(path);
                    if (!dir)
                    {
                        return -1;
                    }
                    while ((ptr = readdir(dir)) != NULL)
                    {
                        if (!ptr->d_name)
                        {
                            continue;
                        }
                        if (ptr->d_name[0] == '.')
                        {
                            continue;
                        }
                        snprintf(sbuf, sizeof(sbuf)-1,"%s%s",path, ptr->d_name);
                        remove(sbuf);                     }
                                        strcpy(sbuf, "drawpad init");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                    sbuf,  strlen( sbuf)+1);

                }
            }
            else            {
                                if(A4SGUIData->DrawPadEnable == 1)
                {
                    if(A4SGUIData->drawpad.info.winopened == 1)
                    {
                        strcpy(sbuf, "drawpad close");
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        sbuf,  strlen( sbuf)+1);

                    }
                }
            }
        }
        else if(!strcasecmp(item, "avalogo"))
        {
            char option1[64] = "";

            ava_mtsubs(param, ' ', 1, option1, sizeof(option1));

            controlID = AVA_Windows_GetControlID(window, "logotext");
            if(controlID >= 0)
            {
                text_st * tx = window->control[controlID];
                if(!strcasecmp(option1, "hide") && tx->basicInfo.visible == 1)
                {
                    tx->basicInfo.visible = 0;
                }
                else if( tx->basicInfo.visible == 0)
                {
                    tx->basicInfo.visible = 1;
                }
            }
            controlID = AVA_Windows_GetControlID(window, "aboutlogotext");
            if(controlID >= 0)
            {
                text_st * tx = window->control[controlID];
                if(!strcasecmp(option1, "hide") && tx->basicInfo.visible == 1)
                {
                    tx->basicInfo.visible = 0;
                }
                else if( tx->basicInfo.visible == 0)
                {
                    tx->basicInfo.visible = 1;
                }
            }
        }
    }
    else if (!strcasecmp(group, "INTERA"))
    {
        if (!strcasecmp(item, "interaClassData"))
        {
            if (!strcasecmp(param, "Reload"))
            {
                char sbuf[64] = "";
                sprintf(sbuf, "%d", INTERADATA_RELOAD);
                SendCommandToWindow(GUIData->windows, &interactClass, CONTROL_WINDOW_OTHER, sbuf);
            }
        }
        else if (!strcasecmp(item, "interactState"))
        {
            if(A4SGUIData->PanelInteraSwitch == 0 && (atoi(param) == 0 || atoi(param) == 1 || atoi(param) == 4))
            {
                int flag;
                char name[64] = "";
                button_st *bt;

                if(window)
                {
                    AVA_GUI_LockScreen( &GUIData->GUIHandle, 1);
                    usleep(100000);
                }
                if(A4SGUIData->interaMode == 1)
                {
                    BlendTypeControlEnable(windows, window, 0);
                    a4s_osd_disableUI(windows, window);
                }
                else if(A4SGUIData->interaMode == 4)
                {
                    BlendTypeControlEnable(windows, window, 0);
                    a4s_osd_enableUI(windows, window);
                }
                else if(A4SGUIData->interaMode == 0)
                {
                    BlendTypeControlEnable(windows, window, 1);
                    a4s_osd_enableUI(windows, window);
                }

                char url[128] = "";
                char url2[128] = "";
                char sbuf[128] = "";
                char sbuf2[128] = "";

                GetVPValue(GUIData, "interaSource", url, sizeof(url));
                GetConfigData(GUIData, "gui", "interactSource", url2, sizeof(url2));
                if(strlen(ava_mtsubs( url, '/', 1, sbuf, sizeof( sbuf))) > 0)
                {
                    if(strlen(ava_mtsubs( url2, ' ', 1, sbuf2, sizeof( sbuf2))) > 0 && !gui_param_exist( sbuf, sbuf2, ','))
                    {
                    }
                    else
                    {
                        ava_mtsubs( sbuf, ',', 1, sbuf2, sizeof( sbuf2));
                    }

                    if(A4SGUIData->interaMode != 0)
                    {
                        if(atoi(sbuf2) - 1 == 0)
                        {
                            gui_interaction_WIFI_Enable(windows, window, 0, 1);
                            gui_interaction_WIFI_Enable(windows, window, 2, 0);
                        }
                        else
                        {
                            gui_interaction_WIFI_Enable(windows, window, 0, 0);
                            gui_interaction_WIFI_Enable(windows, window, 2, 1);
                        }
                    }
                    else
                    {
                        gui_interaction_WIFI_Enable(windows, window, 0, 1);
                        gui_interaction_WIFI_Enable(windows, window, 2, 1);
                    }
                }

                if(strlen(ava_mtsubs( url, '/', 2, sbuf, sizeof( sbuf))) > 0)
                {
                    if(strlen(ava_mtsubs( url2, ' ', 2, sbuf2, sizeof( sbuf2))) > 0 && !gui_param_exist( sbuf, sbuf2, ','))
                    {
                    }
                    else
                    {
                        ava_mtsubs( sbuf, ',', 1, sbuf2, sizeof( sbuf2));
                    }
                    if(A4SGUIData->interaMode != 0)
                    {
                        if(atoi(sbuf2) - 1 == 1)
                        {
                            gui_interaction_WIFI_Enable(windows, window, 1, 1);
                            gui_interaction_WIFI_Enable(windows, window, 3, 0);
                        }
                        else
                        {
                            gui_interaction_WIFI_Enable(windows, window, 1, 0);
                            gui_interaction_WIFI_Enable(windows, window, 3, 1);
                        }
                        gui_interaction_WIFI_Enable(windows, window, 4, 0);
                    }
                    else
                    {
                        gui_interaction_WIFI_Enable(windows, window, 1, 1);
                        gui_interaction_WIFI_Enable(windows, window, 3, 1);
                        gui_interaction_WIFI_Enable(windows, window, 4, 1);
                    }
                }

                flag = 0;
                for (i=1; i<=3; i++)
                {
                    sprintf(  name, "atbutton_%d", i);
                    controlID = AVA_Windows_GetControlID(window, name);
                    if(controlID >= 0)
                    {
                        bt = window->control[controlID];
                        if(!strcasecmp(bt->caption+strlen(bt->caption)-2, "_3"))
                            flag = i;
                    }
                }
                if(A4SGUIData->interaMode == 0)
                    autotrackchange(GUIData->windows, window, flag);
                else
                {
                    PTZControlEnable( windows, window, 1);

                    controlID = AVA_Windows_GetControlID(window, "bm00");
                    if(controlID >= 0)
                    {
                        bt = window->control[controlID];
                        if(bt->basicInfo.enabled == 1)
                            BlendModeControlEnable(windows, window, 0);
                    }
                }

                controlID = AVA_Windows_GetControlID( window, "InteratSource3");
                if(controlID >= 0)
                {
                    bt = window->control[controlID];
                    bt->basicInfo.frame.backgroundColor = BUTTOM_BGCOLOR;
                    int controlID2 = AVA_Windows_GetControlID(window, "panel_8");
                    if(controlID2 >= 0)
                    {
                        panel_st *pl = window->control[controlID2];
                        if(window && pl->basicInfo.visible == 1)
                            AVA_Windows_RedrawControl(windows, window, controlID);
                    }
                }

                controlID = AVA_Windows_GetControlID(window, "systemAppCmdBt1");
                if(controlID >= 0)
                {
                    bt = window->control[controlID];
                    if(bt->basicInfo.enabled != !A4SGUIData->interaMode)
                    {
                        bt->basicInfo.enabled = (A4SGUIData->interaMode == 0)?1:0;
                        if(window && bt->basicInfo.visible == 1)
                            AVA_Windows_RedrawControl(windows, window, controlID);
                    }
                }

                controlID = AVA_Windows_GetControlID(window, "systemAppCmdBt5");
                if(controlID >= 0)
                {
                    bt = window->control[controlID];
                    if(bt->basicInfo.enabled != !A4SGUIData->interaMode)
                    {
                        bt->basicInfo.enabled = (A4SGUIData->interaMode == 0)?1:0;
                        if(window && bt->basicInfo.visible == 1)
                            AVA_Windows_RedrawControl(windows, window, controlID);
                    }
                }

                coverAreaSetControl(windows, window);

                InteractionControl_refresh(windows, window);

                if(window)
                    AVA_GUI_LockScreen( &GUIData->GUIHandle, 0);
            }
        }
        else if (!strcasecmp(item, "Network"))
        {
            controlID = AVA_Windows_GetControlID( window, "NetworkAdaptive");
            if ( controlID >= 0)
            {
                checkBox_st *cb = window->control[controlID];
                if(!strcasecmp(param, "on") && cb->value != 1)
                    cb->value = 1;
                else if(!strcasecmp(param, "off") && cb->value != 0)
                    cb->value = 0;
                else
                    return 0;
                int id = AVA_Windows_GetControlID(window, "panel_8");
                if(id >= 0)
                {
                    panel_st *pl = window->control[id];
                    if(pl->basicInfo.visible == 1)
                        AVA_Windows_RedrawControl(windows, window, controlID);
                }
            }
        }
        else if (!strcasecmp(item, "interaNetwork"))
        {
            int num;
            char option1[64] = "";
            char name[64] = "";

            num = atoi(ava_mtsubs( param, ' ', 1, option1, sizeof( option1)));
            if(num < 1 || num > 3 || num > A4SGUIData->interactRTSPNum)
                return -1;
            ava_mtsubs( param, ' ', 2, option1, sizeof( option1));

            sprintf(name, "interactionTips%d", num);
            SetDataToUI(window, name, option1);
            controlID = AVA_Windows_GetControlID( window, "panel_8");
            if (controlID >= 0)
            {
                panel_st *pl = window->control[controlID];
                if(pl->basicInfo.visible == 1 && pl->basicInfo.enabled == 1 )
                {
                    AVA_Windows_RedrawControlByName(windows, window, name);
                }
            }
        }
        else if (!strcasecmp(item, "interakeyon"))
        {
            gui_interactSource_Key_On( windows,  window, atoi(param));

            if(A4SGUIData->MainArrange[7] <= 0 && A4SGUIData->PanelInteraSwitch == 1)
            {
                char sbuf[64] = "";
                char msgbuf[64] = "";

                GetVPValue(GUIData, "InteraKPV31Enable", sbuf, sizeof(sbuf));
                if(!strcasecmp(sbuf, "enable"))
                {
                    switch(atoi(param))
                    {
                        case 1:
                            strcpy(msgbuf, "mode v1 8");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf)+1);
                            strcpy(msgbuf, "mode v2 7");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf)+1);
                            strcpy(msgbuf, "pc axv31 layout 1");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_HTTP, COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf)+1);
                            break;
                        case 2:
                            strcpy(msgbuf, "mode V1&2 15");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf)+1);
                            strcpy(msgbuf, "pc axv31 layout 2");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_HTTP, COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf)+1);
                            break;
                    }
                }
            }
        }
        else if (!strcasecmp(item, "allstart"))
        {
            gui_interactionConnect_ControlAll(windows,  window, 1);
        }
        else if (!strcasecmp(item, "allstop"))
        {
            gui_interactionConnect_ControlAll(windows,  window, 0);
        }
        else if (!strcasecmp(item, "interaconnect"))
        {
            int rtspnum;
            char name[64] = "";
            char option1[64] = "";
            char sbuf[128] = "";
            char sbuf2[128] = "";
            ava_mtsubs( param, ' ', 1, option1, sizeof( option1));

            memset(name, 0, sizeof(name));
            rtspnum = 0;
            for(i=0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
            {
                if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "SIP") || !strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, "RTSP", 4) )
                {
                    if(!strncasecmp(GUIData->product->videoSignalInfo.videoResource[i].namec, "WIFI", 4))
                        continue;
                    rtspnum ++;
                    if(!strcasecmp(GUIData->product->videoSignalInfo.videoResource[i].name, option1))
                    {
                        sprintf(name, "interactConnectStatus%d", rtspnum);
                        break;
                    }
                    if(rtspnum >= A4SGUIData->interactRTSPNum)
                        break;
                }
            }
            if(strlen(name) < 5)
                return 0;
            controlID = AVA_Windows_GetControlID( window, name);
            if ( controlID >= 0)
            {
                strcpy(sbuf, A4SGUIData->interaVideo[rtspnum+3]);

                text_st *tx = window->control[controlID];
                ava_mtsubs( param, ' ', 2, option1, sizeof( option1));
                if(!strcasecmp(option1, "connected"))
                {
                    strcpy(tx->caption, "ic_connect");
                    gui_interactionConnect_Enable(windows, window, rtspnum, 2);
                }
                else if(!strcasecmp(option1, "disconnected"))
                {
                    strcpy(tx->caption, "ic_disconnect");
                    gui_interactionConnect_Enable(windows, window, rtspnum, 1);
                }
                else if(!strncasecmp(option1, "connectstate", 12))
                {
                    sprintf(tx->caption, "ic_state%02d", atoi(option1+12));
                    gui_interactionConnect_Enable(windows, window, rtspnum, 2);
                }
                else
                {
                    if(!strcasecmp(option1, "versionlow"))
                    {
                        getLanguageString( MAINWINDOW_INTERATION_TIPS3, GUIData->currentLanguage, sbuf2);
                        gui_Tips_display(GUIData, TIPS_OPEN, sbuf2);
                    }
                    return 0;
                }

                int id = AVA_Windows_GetControlID(window, "panel_8");
                if(id>=0)
                {
                    panel_st *pl = window->control[id];
                    if(window && pl->basicInfo.visible == 1)
                        AVA_Windows_RedrawControl(windows, window, controlID);
                }

                if(A4SGUIData->PanelInteraSwitch == 1)
                {
                    id = AVA_Windows_GetControlID(window, "panel_interaction2");
                    if(id >= 0)
                    {
                        panel_st *pl = window->control[id];
                        if(window && pl->basicInfo.visible == 1)
                            AVA_Windows_RedrawControl(windows, window, controlID);
                    }
                }
                                controlID = AVA_Windows_GetControlID(window, "adminbutton_3");
                if(controlID >= 0)
                {
                    button_st *bt = window->control[controlID];
                    if(bt->basicInfo.visible == 1)
                    {
                        if(strlen(A4SGUIData->interaSignal) == 0 && bt->basicInfo.enabled == 0)
                        {
                            if(window)
                            {
                                AVA_GUI_LockScreen( &GUIData->GUIHandle, 1);
                                usleep(100000);
                            }
                            AdminButtonEnable(windows, window, 3);
                            if(window)
                                AVA_GUI_LockScreen( &GUIData->GUIHandle, 0);
                        }
                        else if(strlen(A4SGUIData->interaSignal) > 0 && bt->basicInfo.enabled == 1)
                        {
                            if(window)
                            {
                                AVA_GUI_LockScreen( &GUIData->GUIHandle, 1);
                                usleep(100000);
                            }
                            AdminButtonDisable(windows, window, 3);
                            if(window)
                                AVA_GUI_LockScreen( &GUIData->GUIHandle, 0);
                        }
                    }
                }

                                controlID = AVA_Windows_GetControlID(window, "systemAppCmdBt15");
                if(controlID >= 0)
                {
                    button_st *bt = window->control[controlID];
                    if(strlen(A4SGUIData->interaSignal) == 0 && bt->basicInfo.enabled == 0 && bt->basicInfo.visible == 1)
                    {
                        bt->basicInfo.enabled = 1;
                    }
                    else if(strlen(A4SGUIData->interaSignal) > 0 && bt->basicInfo.enabled == 1 && bt->basicInfo.visible == 1)
                    {
                        bt->basicInfo.enabled = 0;
                    }

                    AVA_Windows_RedrawControlByName(windows, window, "systemAppCmdBt15");
                }

                                controlID = AVA_Windows_GetControlID(window, "systemAppCmdBt20");
                if(controlID >= 0)
                {
                    button_st *bt = window->control[controlID];
                    if(strlen(A4SGUIData->interaSignal) == 0 && bt->basicInfo.enabled == 0 && bt->basicInfo.visible == 1)
                    {
                        bt->basicInfo.enabled = 1;
                    }
                    else if(strlen(A4SGUIData->interaSignal) > 0 && bt->basicInfo.enabled == 1 && bt->basicInfo.visible == 1)
                    {
                        bt->basicInfo.enabled = 0;
                    }

                    AVA_Windows_RedrawControlByName(windows, window, "systemAppCmdBt20");
                }
            }
        }
        else if (!strcasecmp(item, "regstate"))
        {
            controlID = AVA_Windows_GetControlID( window, "InteractionRegstate");
            if ( controlID >= 0)
            {
                text_st *tx = window->control[controlID];
                if(!strcasecmp(param, "disconnected"))
                    getLanguageString( MAINWINDOW_INTERATION_REGSTATE_TIPS1, GUIData->currentLanguage, tx->caption);
                else if(!strcasecmp(param, "resolving"))
                    getLanguageString( MAINWINDOW_INTERATION_REGSTATE_TIPS2, GUIData->currentLanguage, tx->caption);
                else if(!strcasecmp(param, "connecting"))
                    getLanguageString( MAINWINDOW_INTERATION_REGSTATE_TIPS3, GUIData->currentLanguage, tx->caption);
                else if(!strcasecmp(param, "connected"))
                    getLanguageString( MAINWINDOW_INTERATION_REGSTATE_TIPS4, GUIData->currentLanguage, tx->caption);
                else if(!strcasecmp(param, "sameport"))
                    getLanguageString( MAINWINDOW_INTERATION_REGSTATE_TIPS5, GUIData->currentLanguage, tx->caption);
                else if(!strcasecmp(param, "sameaccount"))
                    getLanguageString( MAINWINDOW_INTERATION_REGSTATE_TIPS6, GUIData->currentLanguage, tx->caption);
                else
                    return 0;
                int id = AVA_Windows_GetControlID(window, "panel_8");
                if(id>=0)
                {
                    panel_st *pl = window->control[id];
                    if(pl->basicInfo.visible == 1)
                        AVA_Windows_RedrawControl(windows, window, controlID);
                }
            }
        }
    }

    return 0;
}

int executeLocalGUICommand(void* localData, int srcId, int dstId, int cmd, char *buffer)
{
    char buf[DefaultMessageQueueBufferSize*2] = "";
    char command[256] = "";
    GUI_DATA * GUIData = (GUI_DATA *)localData;

    switch( cmd)
    {
        case COMMAND_GUI_INIT:
        {
            int i, j;
            char *pbuf;
            char sbuf[128] = "";
            char path[128] = "";
            char msgbuf[128] = "";
            char option1[128] = "";
            static char MainSetup[9][32] = {"common", "logo", "subtitle", "blendmode", "blendstyle", "ptz", "volume", "interaction", "about"};

            GUIData->mainWindow = &MainWindow;

            GUIData->GUITimer = 100;

            A4SGui_Data * A4SGUIData = malloc( sizeof(A4SGui_Data));
            memset(A4SGUIData, 0, sizeof(A4SGui_Data));
            GUIData->LocalGUIData = A4SGUIData;
            A4SGUIData->recStartTime = -1;
            A4SGUIData->rtspStartTime = -1;
            A4SGUIData->pocswitch = -1;
            A4SGUIData->pocstate = -1;
            A4SGUIData->rmmaUserID = -1;

            if (strcasecmp(GUIData->devName, "sv06"))
            {
                pbuf = AVA_VP_GetValue(GUIData->product->paramValues, "[gui]currentLanguage");
                if ( pbuf && strlen(pbuf) > 0)
                {
                    strcpy(sbuf,  pbuf);
                    if(!strcasecmp(sbuf, "ENGLISH") || !strcasecmp(sbuf, "E"))
                        GUIData->currentLanguage = LANGUAGE_ENGLISH;
                    else if(!strcasecmp(sbuf, "SIMPLIFIEDCHINESE") || !strcasecmp(sbuf, "S"))
                        GUIData->currentLanguage = LANGUAGE_SIMPLIFIEDCHINESE;
                    else if(!strcasecmp(sbuf, "TRIDITIONALCHINESE") || !strcasecmp(sbuf, "T"))
                        GUIData->currentLanguage = LANGUAGE_TRIDITIONALCHINESE;
                    else
                    {
                        GUIData->currentLanguage = LANGUAGE_SIMPLIFIEDCHINESE;
                        SetConfigData(GUIData, "gui", "currentLanguage", "SIMPLIFIEDCHINESE");
                    }
                }
                else
                {
                    GUIData->currentLanguage = LANGUAGE_SIMPLIFIEDCHINESE;
                    SetConfigData(GUIData, "gui", "currentLanguage", "SIMPLIFIEDCHINESE");
                }
            }

            GetVPValue(GUIData, "VgaDisplayName", A4SGUIData->vgaDisplayName, sizeof(A4SGUIData->vgaDisplayName));

            GetVPValue(GUIData, "MachineType", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "education"))
            {
                A4SGUIData->MachineType = 0;
                GetVPValue(GUIData, "medicalon", sbuf, sizeof(sbuf));
                if(atoi(sbuf) == 1)
                    A4SGUIData->MachineType = 3;
            }
            else if(!strcasecmp(sbuf, "court"))
                A4SGUIData->MachineType = 1;
            else if(!strcasecmp(sbuf, "stv"))
                A4SGUIData->MachineType = 2;
            else
                A4SGUIData->MachineType = 0;

            GetVPValue(GUIData, "SpecialMachine", sbuf, sizeof(sbuf));
            if(strlen(sbuf) > 2)
                strcpy(A4SGUIData->SpecialMachine, sbuf);

            GetVPValue(GUIData, "PanelSetupSwitch", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "disable"))
                A4SGUIData->PanelSetupSwitch = 0;
            else
                A4SGUIData->PanelSetupSwitch = 1;

            GetVPValue(GUIData, "PanelInteraSwitch", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "enable"))
                A4SGUIData->PanelInteraSwitch = 1;
            else
                A4SGUIData->PanelInteraSwitch = 0;

            if (A4SGUIData->PanelSetupSwitch == 1)
            {
                GetVPValue(GUIData, "FunctionSetup", sbuf, sizeof(sbuf));
                if (strlen(sbuf) < 2)
                    strcpy( sbuf, "common,logo,subtitle,blendmode,blendstyle,ptz,volume,about");
            }

            for(j=0; j<9; j++)
            {
                A4SGUIData->MainArrange[j] = 0;
            }
            i = 1;
            while(strlen(ava_mtsubs( sbuf, ',', i, option1, sizeof(option1))) > 1)
            {
                for(j=0; j<9; j++)
                {
                    if(!strcasecmp(option1, MainSetup[j]))
                    {
                        A4SGUIData->MainArrange[j] = i;
                        break;
                    }
                }
                i++;
                if(i>12)
                    break;
            }

            GetVPValue(GUIData, "ElecPtzEnable", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "enable"))
                A4SGUIData->ElecPtzEnable = 1;
            else
                A4SGUIData->ElecPtzEnable = 0;

            GetVPValue(GUIData, "RTSPPtzEnable", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "enable"))
                A4SGUIData->RTSPPtzEnable = 1;
            else
                A4SGUIData->RTSPPtzEnable = 0;

            GetVPValue(GUIData, "ExtendBoard", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "U15"))
                A4SGUIData->ExtendBoard = 1;
            else
                A4SGUIData->ExtendBoard = 0;

            GetVPValue(GUIData, "LCDenable", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "enable"))
                A4SGUIData->LCDenable = 1;
            else
                A4SGUIData->LCDenable = 0;

            GetVPValue(GUIData, "VGAMODE", sbuf, sizeof(sbuf));
            if ( strlen(sbuf) > 1)
                strcpy( A4SGUIData->vgamode, sbuf);
            else
                strcpy( A4SGUIData->vgamode, "V5");

            GetVPValue(GUIData, "DrawPadEnable", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "enable"))
                A4SGUIData->DrawPadEnable = 1;
            else
                A4SGUIData->DrawPadEnable = 0;

            GetVPValue(GUIData, "KeyingWindow2", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "SDI3"))
                A4SGUIData->keyingmask = 0;
            else
                A4SGUIData->keyingmask = 1;

            GetVPValue(GUIData, "KeyingThreshold4Enable", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "disable"))
                A4SGUIData->KeyingThreshold4Enable = 0;
            else
                A4SGUIData->KeyingThreshold4Enable = 1;

            GetVPValue(GUIData, "KeyingEnable", sbuf, sizeof(sbuf));
            if(!strcasecmp(sbuf, "enable"))
                A4SGUIData->KeyingEnable = 1;
            else
                A4SGUIData->KeyingEnable = 0;

            if(A4SGUIData->KeyingEnable == 1)
            {
#if defined(__3531A__)
                FPGA_MAT_Enabled(1);
                GetConfigData(GUIData, "gui", "KeyingThreshold1", sbuf, sizeof(sbuf));
                if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 100)
                    FPGA_MAT_SetThreshold(atoi(sbuf));
                GetConfigData(GUIData, "gui", "KeyingThreshold3", sbuf, sizeof(sbuf));
                if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 200)
                    FPGA_MAT_SetThreshold2(atoi(sbuf));
                GetConfigData(GUIData, "gui", "KeyingBG1", sbuf, sizeof(sbuf));
                if(!strcasecmp(sbuf, "GREEN"))
                    FPGA_MAT_SetBackGroundColor(1);
                else
                    FPGA_MAT_SetBackGroundColor(0);

                GetVPValue(GUIData, "KeyingHDMI3VoEnable", sbuf, sizeof(sbuf));
                if(!strcasecmp(sbuf, "enable"))
                {
                    GetConfigData(GUIData, "gui", "KeyingHDMI3Vo", sbuf, sizeof(sbuf));
                    if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 2)
                        FPGA_MAT_SetHDMI3VO(atoi(sbuf));

                    GetConfigData(GUIData, "gui", "KeyingHDMI3VoColor", sbuf, sizeof(sbuf));
                    if ( strlen(sbuf) > 0 && atoi(sbuf) >= 100 && atoi(sbuf) <= 200)
                        FPGA_MAT_SetHDMI3VOCOLOR(atoi(sbuf));

                    GetConfigData(GUIData, "gui", "KeyingHDMI3VoML", sbuf, sizeof(sbuf));
                    if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 128)
                        FPGA_MAT_SetHDMI3VOMAINLUMINANCE(atoi(sbuf));

                    GetConfigData(GUIData, "gui", "KeyingHDMI3VoSL", sbuf, sizeof(sbuf));
                    if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 128)
                        FPGA_MAT_SetHDMI3VOSUBLUMINANCE(atoi(sbuf));
                }
#else
                FPGA_MAT_Enabled(0, 1);
                FPGA_MAT_Enabled(1, 1);
                FPGA_MAT_Enabled(2, 1);
                FPGA_MAT_SetBackGround(0, 0);
                FPGA_MAT_SetBackGround(1, 0);
                FPGA_MAT_SetBackGround(2, 0);

                GetConfigData(GUIData, "gui", "KeyingThreshold1", sbuf, sizeof(sbuf));
                if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 100)
                    FPGA_MAT_SetThreshold( 0, atoi(sbuf));
                GetConfigData(GUIData, "gui", "KeyingThreshold2", sbuf, sizeof(sbuf));
                if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 100)
                    FPGA_MAT_SetThreshold( 1+A4SGUIData->keyingmask, atoi(sbuf));
                GetConfigData(GUIData, "gui", "KeyingThreshold3", sbuf, sizeof(sbuf));
                if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 200)
                    FPGA_MAT_SetThreshold2( 0, atoi(sbuf));
                if(A4SGUIData->KeyingThreshold4Enable == 1)
                {
                    GetConfigData(GUIData, "gui", "KeyingThreshold4", sbuf, sizeof(sbuf));
                    if ( strlen(sbuf) > 0 && atoi(sbuf) >= 0 && atoi(sbuf) <= 200)
                        FPGA_MAT_SetThreshold2( 1+A4SGUIData->keyingmask, atoi(sbuf));
                }

                GetConfigData(GUIData, "gui", "KeyingBG1", sbuf, sizeof(sbuf));
                if(!strcasecmp(sbuf, "GREEN"))
                    FPGA_MAT_SetBackGroundColor( 0, 1);
                else
                    FPGA_MAT_SetBackGroundColor( 0, 0);
                GetConfigData(GUIData, "gui", "KeyingBG2", sbuf, sizeof(sbuf));
                if(!strcasecmp(sbuf, "GREEN"))
                    FPGA_MAT_SetBackGroundColor( 1+A4SGUIData->keyingmask, 1);
                else
                    FPGA_MAT_SetBackGroundColor( 1+A4SGUIData->keyingmask, 0);
#endif
            }

            GetVPValue(GUIData, "PTZNum", sbuf, sizeof(sbuf));
            if ( strlen(sbuf) > 0)
                A4SGUIData->ptznum = atoi(sbuf);
            else
                A4SGUIData->ptznum = 6;
            if(A4SGUIData->ptznum > GUIData->product->monitorVideos)
                A4SGUIData->ptznum = GUIData->product->monitorVideos;

            j=-1;
            for(i=1; i<=16; i++)
            {
                j++;
                if(j < GUIData->product->videoSignalInfo.videoResourceCount)
                {
                    if(strncasecmp(GUIData->product->videoSignalInfo.videoResource[j].name, "SDI", 3)
                        && strncasecmp(GUIData->product->videoSignalInfo.videoResource[j].name, "RTSP", 4)
                        && strncasecmp(GUIData->product->videoSignalInfo.videoResource[j].name, "SIP", 3)
                        && strncasecmp(GUIData->product->videoSignalInfo.videoResource[j].name, "H323", 4)
                        && strncasecmp(GUIData->product->videoSignalInfo.videoResource[j].name, "HDMI", 4))
                    {
                        sprintf(path, "signalswitch%02d", j+1);
                        GetConfigData(GUIData, "vrm", path, sbuf, sizeof(sbuf));
                        if(strcasecmp(sbuf, "0"))
                            SetConfigData(GUIData, "vrm", path, "0");
                        sprintf(path, "signalurl%02d", j+1);
                        GetConfigData(GUIData, "vrm", path, sbuf, sizeof(sbuf));
                        if(strlen(sbuf) > 0)
                            SetConfigData(GUIData, "vrm", path, "");
                    }
                }
                else
                    break;
            }

            GetConfigData(GUIData, "gui", "NetworkParam", sbuf, sizeof(sbuf));
            if(atoi(sbuf) >= 1024 && atoi(sbuf) <= 4096)
                A4SGUIData->NetworkParam = atoi(sbuf) / 512 *512;
            else
            {
                A4SGUIData->NetworkParam = 2048;
                SetConfigData(GUIData, "gui", "NetworkParam", "2048");
            }

            for(i=0; i<6; i++)
            {
                sprintf(path, "presetzoom%d", i+1);
                GetConfigData(GUIData, "gui", path, sbuf, sizeof(sbuf));
                for(j=0; j<4; j++)
                {
                    if(strlen(ava_mtsubs( sbuf, ' ', j+1, option1, sizeof( option1))) > 0)
                        A4SGUIData->presetzoom[i][j] = atoi(option1);
                    else
                        A4SGUIData->presetzoom[i][j] = j*250;
                }
            }

            if(A4SGUIData->MainArrange[7] > 0)
            {
                int mode;
                char url[64] = "";
                char url2[64] = "";

                GetVPValue(GUIData, "interactRTSPNum", sbuf, sizeof(sbuf));
                if(strlen(sbuf) > 0)
                    A4SGUIData->interactRTSPNum = atoi(sbuf);
                else
                    A4SGUIData->interactRTSPNum = 3;

                GetVPValue(GUIData, "interaVideo", sbuf, sizeof(sbuf));
                if(strlen(sbuf) > 0)
                {
                    i=1;
                    while(strlen(ava_mtsubs( sbuf, ',', i, option1, sizeof( option1))) > 0)
                    {
                        strcpy(A4SGUIData->interaVideo[i-1], option1);
                        i++;
                        if(i > 10)
                            break;
                    }
                }

                mode = 0;
                GetVPValue(GUIData, "interaSource", url, sizeof(url));
                GetConfigData(GUIData, "gui", "interactSource", url2, sizeof(url2));
                if(strlen(ava_mtsubs( url, '/', 1, sbuf, sizeof( sbuf))) > 0)
                {
                    if(strlen(ava_mtsubs( url2, ' ', 1, option1, sizeof( option1))) > 0 && !gui_param_exist( sbuf, option1, ','))
                    {
                        sprintf(A4SGUIData->interaVideo[0], "%d", GUIData->product->videoWindowInfo.videoWindow[atoi(option1)-1].vpssID);
                        mode = atoi(ava_mtsubs( sbuf, ' ', 1, option1, sizeof( option1)));
                    }
                    else
                    {
                        ava_mtsubs( sbuf, ',', 1, option1, sizeof( option1));
                        sprintf(A4SGUIData->interaVideo[0], "%d", GUIData->product->videoWindowInfo.videoWindow[atoi(option1)-1].vpssID);
                    }
                }
                if(strlen(ava_mtsubs( url, '/', 2, sbuf, sizeof( sbuf))) > 0)
                {
                    if(strlen(ava_mtsubs( url2, ' ', 2, option1, sizeof( option1))) > 0 && !gui_param_exist( sbuf, option1, ','))
                    {
                        sprintf(A4SGUIData->interaVideo[1], "%d", GUIData->product->videoWindowInfo.videoWindow[atoi(option1)-1].vpssID);
                    }
                    else
                    {
                        ava_mtsubs( sbuf, ',', 1, option1, sizeof( option1));
                        sprintf(A4SGUIData->interaVideo[1], "%d", GUIData->product->videoWindowInfo.videoWindow[atoi(option1)-1].vpssID);
                    }
                }

                sprintf(msgbuf, "set FILE5 %d", atoi(A4SGUIData->interaVideo[7]) + 1);
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VRM, COMMAND_BY_CHAR,
                                msgbuf, strlen( msgbuf)+1);

                if(mode <= 0)
                    mode = 1;
                else if(mode > GUIData->product->monitorVideos)
                    mode = GUIData->product->monitorVideos;

                GetConfigData(GUIData, "intera", "interactState", sbuf, sizeof(sbuf));
                A4SGUIData->interaMode = atoi(sbuf);

                GetConfigData(GUIData, "gui", "interaDispMode", sbuf, sizeof(sbuf));
                A4SGUIData->interaDispMode = atoi(sbuf);

                GetConfigData(GUIData, "gui", "interaNetLabelEnable", sbuf, sizeof(sbuf));
                A4SGUIData->interaNetLabelEnable = atoi(sbuf);

                GetConfigData(GUIData, "rsc", "ProxySwitch", sbuf, sizeof(sbuf));
                A4SGUIData->interaProxySwitch = atoi(sbuf);

                GetConfigData(GUIData, "at", "trackmode", sbuf, sizeof(sbuf));
                if(!strcasecmp(sbuf, "all"))
                    A4SGUIData->interaAT = 0;
                else if(!strcasecmp(sbuf, "teacher"))
                    A4SGUIData->interaAT = 1;
                else if(!strcasecmp(sbuf, "student"))
                    A4SGUIData->interaAT = 2;
                else if(!strcasecmp(sbuf, "none"))
                    A4SGUIData->interaAT = 3;
                else
                    A4SGUIData->interaAT = 0;

                if(A4SGUIData->interaMode == 0)
                    strcpy(msgbuf, "mask 3");
                else if(A4SGUIData->interaAT == 3)
                    strcpy(msgbuf, "mask 0");
                else if(A4SGUIData->interaMode == 1)
                    strcpy(msgbuf, "mask 12");
                else if(A4SGUIData->interaMode == 4)
                    strcpy(msgbuf, "mask 14");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                msgbuf, strlen( msgbuf)+1);

                if(A4SGUIData->interaMode == 1 || A4SGUIData->interaMode == 4)
                {
                    strcpy(msgbuf, "interaswitch 1");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_ARM, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "type none");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "start 12");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    if(A4SGUIData->interaMode == 1)
                    {
                        strcpy(msgbuf, "vpss 4 34");
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf)+1);
                        sprintf(msgbuf, "mode v%d 12", mode);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf)+1);
                        sprintf(msgbuf, "modex 3 M1 %s", A4SGUIData->interaVideo[4]);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf)+1);
                        Gui_MainOsdTarget_Change(GUIData, 0);
                        Gui_InteraOsd_Change(GUIData, 1, -1);
                    }
                    else if(A4SGUIData->interaMode == 4)
                    {
                        sprintf(msgbuf, "vpss 4 %d", (atoi(A4SGUIData->interaVideo[7])<<8)+34);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf)+1);
                        sprintf(msgbuf, "mode v%d 14", mode);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf)+1);
                        if(A4SGUIData->interaProxySwitch == 2)
                        {
                            sprintf(msgbuf, "modex 8 M1 %s", A4SGUIData->interaVideo[0]);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf) + 1 );
                            sprintf(msgbuf, "modex 16 M1 %s", A4SGUIData->interaVideo[1]);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf) + 1 );
                            sprintf(msgbuf, "modex 32 M1 %s", A4SGUIData->interaVideo[2]);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VB,  COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf) + 1 );
                        }
                        if(A4SGUIData->interactRTSPNum > 1)
                            sprintf(msgbuf, "modex 1 M1 %s", A4SGUIData->interaVideo[7]);
                        else
                            sprintf(msgbuf, "modex 1 1+1 %s %s", A4SGUIData->interaVideo[4], A4SGUIData->interaVideo[7]);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf,  strlen( msgbuf)+1);
                        Gui_MainOsdTarget_Change(GUIData, 1);
                        Gui_InteraOsd_Change(GUIData, 1, -1);
                    }
                }
                else
                {
                    strcpy(msgbuf, "interaswitch 0");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_ARM, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "mode -1 12");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "stop 12");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "mode v1");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    strcpy(msgbuf, "vpss 4 34");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                    msgbuf,  strlen( msgbuf)+1);
                    Gui_MainOsdTarget_Change(GUIData, 0);
                    Gui_InteraOsd_Change(GUIData, 0, -1);
                }

                                GetConfigData(GUIData, "gui", "interaOutputEnable", buf, sizeof(buf)-1);                if( atoi(buf) == 1 )
                {
                    if(A4SGUIData->interaMode == 0)
                    {
                        sprintf( sbuf, "output %d %d %s %s", 0, 1, "1080P60", "1080P60");                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        sbuf,  strlen( sbuf)+1);
                    }
                    else if(A4SGUIData->interaMode == 1)
                    {
                        sprintf( sbuf, "output %d %d %s %s", 1, 3, "1080P60", "1080P60");                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        sbuf,  strlen( sbuf)+1);
                    }
                    else if(A4SGUIData->interaMode == 4)
                    {
                        GetVPValue(GUIData, "DualStreamEnable", sbuf, sizeof(sbuf));
                        if(!strcasecmp(sbuf, "enable"))
                        {
                            GetConfigData(GUIData,  "intera", "dualstream", sbuf, sizeof(sbuf));
                            if(atoi(sbuf))                             {
                                sprintf( sbuf, "output %d %d %s %s", 1, 3, "1080P60", "1080P60");                                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                                sbuf,  strlen( sbuf)+1);
                            }
                            else
                            {
                                sprintf( sbuf, "output %d %d %s %s", 2, 1, "1080P60", "1080P60");                                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                                sbuf,  strlen( sbuf)+1);
                            }
                        }
                        else
                        {
                            sprintf( sbuf, "output %d %d %s %s", 2, 1, "1080P60", "1080P60");                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                            sbuf,  strlen( sbuf)+1);
                        }
                    }
                }
            }

            if(A4SGUIData->PanelInteraSwitch == 1)
            {
                strcpy(msgbuf, "interaswitch 1");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_ARM, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);
                strcpy(msgbuf, "start 12");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);
                strcpy(msgbuf, "mode v1 8");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);
                strcpy(msgbuf, "mode v2 7");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);
                strcpy(msgbuf, "vpss 4 34");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);
                strcpy(msgbuf, "clearstatus");
                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_RTSP, COMMAND_BY_CHAR,
                                msgbuf,  strlen( msgbuf)+1);
                A4SGUIData->interaMode = 1;
                A4SGUIData->interactRTSPNum = 1;

                GetConfigData(GUIData, "gui", "interaNetLabelEnable", sbuf, sizeof(sbuf));
                A4SGUIData->interaNetLabelEnable = atoi(sbuf);

                Gui_InteraOsd_Change(GUIData, 1, -1);
            }

            int   cueWidth, cueHeight, cueXInterval, cueYInterval, cueLeft, cueTop;
            cueLeft         =  atoi(AVA_VP_GetValue(&GUIData->product->vp, "cueLeft"));
            cueTop          =  atoi(AVA_VP_GetValue(&GUIData->product->vp, "cueTop"));
            cueXInterval    =  atoi(AVA_VP_GetValue(&GUIData->product->vp, "cueXInterval"));
            cueYInterval    =  atoi(AVA_VP_GetValue(&GUIData->product->vp, "cueYInterval"));
            cueWidth        =  atoi(AVA_VP_GetValue(&GUIData->product->vp, "cuePicWidth"));
            cueHeight       =  atoi(AVA_VP_GetValue(&GUIData->product->vp, "cuePicHeight"));
            cueMainInit( GUIData->product, GUIData, GUIData->mainWindow, cueLeft, cueTop, GUIData->product->monitorVideos,
                                GUIData->product->monitorRows, cueWidth, cueHeight, cueXInterval, cueYInterval);

            sprintf(sbuf, "pc osd logosize %d %d", A4SGUIData->LogoWidth, A4SGUIData->LogoHeight);
            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                            sbuf,  strlen( sbuf) + 1 );
            break;
        }
        case COMMAND_GUI_DEINIT:
            if(GUIData->mainWindow)
            {
                A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
                deinitDrawPadBuffer(&A4SGUIData->drawpad);

                cueMainDeinit( GUIData, GUIData->mainWindow);
                free(GUIData->LocalGUIData);
                GUIData->mainWindow = NULL;
            }
            break;
        case CONTROL_WINDOW_TIMER:
        {
            A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
            char sbuf[128] = "";

            if(A4SGUIData->usbhidPtzTime > 0 && gettickcount() - A4SGUIData->usbhidPtzTime > 150)
            {
                gui_ptzControl_byWinID(GUIData, "setzoomstop", A4SGUIData->activePTZWinID, "0");
                A4SGUIData->usbhidPtzTime = 0;
            }

            if(strlen(A4SGUIData->interamsg) > 0 && A4SGUIData->interaMode > 0)
            {
                if(gettickcount() - A4SGUIData->interaTime > 1500)
                    gui_interactSource_Process(GUIData, A4SGUIData->interamsg);
            }

            if(A4SGUIData->LCDenable == 1)
            {
                if ( A4SGUIData->recStartTime != -1)
                {
                    int h, m, s;
                    unsigned long t;
                    t = (gettickcount() - A4SGUIData->recStartTime) / 1000 + A4SGUIData->recBaseLength / 1000;
                    h = t / 3600;
                    m = (t % 3600)  / 60;
                    s = t % 60;
                    sprintf( sbuf, "%02d:%02d:%02d", h, m, s);

                    if(A4SGUIData->lcddisptime <= 0)
                    {
                        A4SGUIData->lcddisptime = 0;
                        sprintf( sbuf, "disp %02d:%02d:%02d", h, m, s);
                        if(strcasecmp(A4SGUIData->lcddispbuf, sbuf+5))
                        {
                            strcpy(A4SGUIData->lcddispbuf, sbuf+5);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_LCD,  COMMAND_BY_CHAR,
                                            sbuf,  strlen( sbuf) + 1 );
                        }
                    }
                    else
                        A4SGUIData->lcddisptime--;
                }
                else if ( A4SGUIData->recBaseLength != 0)
                {
                    if(A4SGUIData->lcddisptime <= 0)
                    {
                        A4SGUIData->lcddisptime = 0;
                        strcpy( sbuf, "disp record pasue");
                        if(strcasecmp(A4SGUIData->lcddispbuf, sbuf+5))
                        {
                            strcpy(A4SGUIData->lcddispbuf, sbuf+5);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_LCD,  COMMAND_BY_CHAR,
                                            sbuf,  strlen( sbuf) + 1 );
                        }
                    }
                    else
                        A4SGUIData->lcddisptime--;
                }
                else
                {
                    if(A4SGUIData->lcddisptime <= 0)
                    {
                        A4SGUIData->lcddisptime = 0;
                        strcpy( sbuf, "disp record ready");
                        if(strcasecmp(A4SGUIData->lcddispbuf, sbuf+5))
                        {
                            strcpy(A4SGUIData->lcddispbuf, sbuf+5);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_LCD,  COMMAND_BY_CHAR,
                                            sbuf,  strlen( sbuf) + 1 );
                        }
                    }
                    else
                        A4SGUIData->lcddisptime--;
                }

            }

            break;
        }
        case COMMAND_KMM_MIDDLEBUTTONDOWN:
        {
            A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
            if(A4SGUIData->activePTZWinID >= GUIData->product->monitorVideos || A4SGUIData->activePTZWinID < 0)
                break;
            int activePTZ = gui_getPtzID_byWinID(GUIData, A4SGUIData->activePTZWinID, NULL);
            if(activePTZ >= 0)
            {
                if(A4SGUIData->ptzZoomValue[activePTZ]/10 < 100)
                    A4SGUIData->ptzZoomValue[activePTZ] = (A4SGUIData->ptzZoomValue[activePTZ]/10+1)*10;
                sprintf( command, "%d", A4SGUIData->ptzZoomValue[activePTZ]);
                gui_ptzControl_byWinID(GUIData, "setzoom", A4SGUIData->activePTZWinID, command);
            }
            break;
        }
        case COMMAND_KMM_MIDDLEBUTTONUP:
        {
            A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
            if(A4SGUIData->activePTZWinID >= GUIData->product->monitorVideos || A4SGUIData->activePTZWinID < 0)
                break;
            int activePTZ = gui_getPtzID_byWinID(GUIData, A4SGUIData->activePTZWinID, NULL);
            if(activePTZ >= 0)
            {
                if(A4SGUIData->ptzZoomValue[activePTZ]/10 > 0)
                    A4SGUIData->ptzZoomValue[activePTZ] = (A4SGUIData->ptzZoomValue[activePTZ]/10-1)*10;
                sprintf( command, "%d", A4SGUIData->ptzZoomValue[activePTZ]);
                gui_ptzControl_byWinID(GUIData, "setzoom", A4SGUIData->activePTZWinID, command);
            }
            break;
        }
        case COMMAND_KMM_DRAWPADMOUSE:
        {
            A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
            if(A4SGUIData->DrawPadEnable != 1)
                break;
            char sbuf[128] = "";
            if(buffer == NULL)
                break;
            kmmMessage kmm;
            memset(&kmm, 0, sizeof(kmm));
            memcpy(&kmm, (kmmMessage *)buffer, sizeof(kmmMessage));

#if DRAWDEBUG
            printf("DRAWPADMOUSE==x=%d==y=%d==cmd=%d==type=%d====\n", kmm.option1, kmm.option2, kmm.cmd, kmm.type);
#endif
            snprintf(sbuf, sizeof(sbuf)-1, "mouse %d %d %d %d", kmm.option1, kmm.option2, kmm.cmd, kmm.type);
            drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
            break;
        }
        case COMMAND_KMM_DRAWPADREMOTEMOUSE:
        {
            printf("Drawpad buffer = %s\n", buffer);
            A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
            if(A4SGUIData->DrawPadEnable != 1)
                break;
            if(buffer == NULL || strlen(buffer) < 3)
                break;
            int remoteId;
            char sbuf[256] = "";
            char srcName[64] = "";
            char option1[64] = "";

            ava_mtsubs(buffer, '/', 1, srcName, sizeof( srcName));
            strncpy(sbuf, buffer+strlen(srcName)+1, 255);

            ava_mtsubs(sbuf, ' ', 1, option1, sizeof(option1));
            remoteId = atoi(option1);
            printf("remoteID = %d\n", remoteId);
            if(remoteId < 1 || remoteId > 3)
                break;
            kmmMessage kmm;

            kmm.type = KMMTYPE_DRAWPAD_PC + remoteId;
            ava_mtsubs(sbuf, ' ', 2, option1, sizeof(option1));
            kmm.cmd = atoi(option1);
            ava_mtsubs(sbuf, ' ', 3, option1, sizeof(option1));
            kmm.option1 = atoi(option1);
            ava_mtsubs(sbuf, ' ', 4, option1, sizeof(option1));
            kmm.option2 = atoi(option1);

            if (kmm.option1 <= 0)
                kmm.option1 = 0;
            if (kmm.option1 >= 1908)
                kmm.option1 = 1908;
            if (kmm.option2 <= 0)
                kmm.option2 = 0;
            if (kmm.option2 >= 1078)
                kmm.option2 = 1078;

            AVA_AddDataToQueue( GUIData->guiDataQueue, COMMAND_KMM_DRAWPADMOUSE, (char *)&kmm,  sizeof( kmmMessage));
            break;
        }
        case COMMAND_GUI_RMMA_SETUSERID:
        {
            A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
            if(buffer == NULL)
                break;

            int num = *(int *)buffer;
            if(num < -1 || num > 3)
                break;
            A4SGUIData->rmmaUserID = num;

            if(A4SGUIData->rmmaUserID >= 0)
            {
                int i, j;
                for(i=0, j=4; i<4 && j<8; i++)
                {
                    if(i == A4SGUIData->rmmaUserID)
                        strcpy(A4SGUIData->rmmaVideo[i], A4SGUIData->interaVideo[0]);
                    else
                    {
                        strcpy(A4SGUIData->rmmaVideo[i], A4SGUIData->interaVideo[j]);
                        j++;
                    }
                }
                strcpy(A4SGUIData->rmmaVideo[4], A4SGUIData->interaVideo[8]);
                strcpy(A4SGUIData->rmmaVideo[5], A4SGUIData->interaVideo[9]);
            }

            printf("\n==COMMAND_GUI_RMMA_SETUSERID==rmmaUserID = %d====%s=%s=%s=%s=%s=%s=\n\n", A4SGUIData->rmmaUserID,
                    A4SGUIData->rmmaVideo[0], A4SGUIData->rmmaVideo[1], A4SGUIData->rmmaVideo[2], A4SGUIData->rmmaVideo[3],
                    A4SGUIData->rmmaVideo[4], A4SGUIData->rmmaVideo[5]);
            break;
        }
        case COMMAND_GUI_RMMA_MSGPROCESS:
        {
            A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
            if(buffer == NULL || A4SGUIData->rmmaUserID < 0)
                break;

            rmmamsg_send rmmamsg;
            memset(&rmmamsg, 0, sizeof(rmmamsg_send));
            memcpy(&rmmamsg, buffer, sizeof(rmmamsg_send));

            int src, dst;
            char option1[512] = "";
            char sbuf[512] = "";
            char url[512] = "";

            strncpy(url, rmmamsg.rmmabuf, rmmamsg.rmmalen);
            printf("=====COMMAND_GUI_RMMA_MSGPROCESS=========%s=============\n", url);
            ava_mtsubs(url, ';', 1, sbuf, sizeof( sbuf));
            src = atoi(ava_mtargs( sbuf, "src", option1, sizeof(option1)));
            ava_mtsubs(url, ';', 2, sbuf, sizeof( sbuf));
            dst = atoi(ava_mtargs( sbuf, "dst", option1, sizeof(option1)));
            if(dst != A4SGUIData->rmmaUserID)
                break;

            ava_mtsubs(url, ';', 3, sbuf, sizeof( sbuf));
            if(!strcmp(sbuf+strlen(sbuf)-4, "\r\n\r\n"))
                *(sbuf+strlen(sbuf)-4) = 0;
            if(!strncasecmp(sbuf, "RMMATOGUI ", strlen("RMMATOGUI ")))
            {
                strcpy(option1, sbuf + strlen("RMMATOGUI "));
                gui_Rmma_MsgProcess(GUIData, src, 1, option1);
            }
            else if(!strncasecmp(sbuf, "RMMAMSG ", strlen("RMMAMSG ")))
            {
                strcpy(option1, sbuf + strlen("RMMAMSG "));
                gui_Rmma_MsgProcess(GUIData, src, 2, option1);
            }
            break;
        }
        case COMMAND_BY_CHAR:
        {
            A4SGui_Data * A4SGUIData = GUIData->LocalGUIData;
            strncpy(buf, buffer, sizeof(buf));
            ava_mtsubs( buf, ' ', 1, command, sizeof( command));

            if ( !strcasecmp( command, "PC"))
            {
                                executeGUICommandParamChange( GUIData->windows, GUIData->mainWindow, GUIData, buffer + strlen( command) + 1);
            }
            else if ( !strcasecmp( command, "CTRL"))
            {
                executeGUICommandControl(GUIData, buffer + strlen( command) + 1);
            }
            else if (!strcasecmp(command, "MainWindow"))
            {
                char param[64] = "";
                ava_mtsubs( buf, ' ', 2, param, sizeof( param));
                if ( GUIData->mainWindow)
                {
                    WindowsStruct * windows = GUIData->windows;
                    if ( !strcasecmp( param, "open"))
                    {
                        if(GUIData->mainWindow && !gui_IfWindowExist(GUIData->windows, GUIData->mainWindow))
                        {
                            AVA_Windows_OpenWindow( windows, GUIData->mainWindow, NULL);
                            A4S_InitOutput( GUIData);
                            if(!strcasecmp(GUIData->devName, "h4m"))
                            {
                                char msgbuf[64] = "";
                                strcpy(msgbuf, "DP501 INIT");
                                AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_SYS, COMMAND_BY_CHAR,
                                                msgbuf,  strlen( msgbuf)+1);
                            }
                        }
                    }
                    else if ( !strcasecmp( param, "close"))
                    {
                    }
                }
            }
            else if (!strcasecmp(command, "Tips"))
            {
                int ret;
                char sbuf[256] = "";
                char item[64] = "";

                if(GUIData->mainWindow && gui_IfWindowExist(GUIData->windows, GUIData->mainWindow))
                {
                    ava_mtsubs( buf, ' ', 2, item, sizeof( item));
                    if ( !strcasecmp( item, "open"))
                        sprintf(sbuf, "%d %s", TIPS_OPEN, buf+strlen(command)+strlen(item)+2);
                    else if (!strcasecmp(item,"REFRESH"))
                        sprintf(sbuf, "%d %s", TIPS_REFRESH, buf+strlen(command)+strlen(item)+2);
                    else if (!strcasecmp(item,"lock"))
                        sprintf(sbuf, "%d %s", TIPS_LOCK, buf+strlen(command)+strlen(item)+2);
                    else if (!strcasecmp(item,"unlock"))
                        sprintf(sbuf, "%d %s", TIPS_UNLOCK, buf+strlen(command)+strlen(item)+2);
                    else if (!strcasecmp(item,"close"))
                        sprintf(sbuf, "%d %s", TIPS_CLOSE, buf+strlen(command)+strlen(item)+2);
                    else
                        sprintf(sbuf, "%d %s", TIPS_OPEN, buf+strlen(command)+1);
                    ret = SendCommandToWindow(GUIData->windows, &Tips, CONTROL_WINDOW_OTHER, sbuf);
                    if(ret && strcasecmp(item, "close"))
                        AVA_Windows_OpenWindow( GUIData->windows, &Tips, (void *)&sbuf);
                }
            }
            else if (!strcasecmp(command, "drawpad"))
            {
                if(!A4SGUIData->DrawPadEnable)
                    return 0;

                char item[64] = "";
                char option1[64] = "";
                char option2[64] = "";
                char sbuf[256] = "", sbuf2[256] = "";

                ava_mtsubs( buf, ' ', 2, item, sizeof( item));
                ava_mtsubs( buf, ' ', 3, option1, sizeof( option1));
                ava_mtsubs( buf, ' ', 4, option2, sizeof( option2));
                if ( !strcasecmp( item, "open"))
                {
                    drawpadMain(GUIData->windows, GUIData->mainWindow, item);

                    if(A4SGUIData->drawpad.data.en_4bit);
                    {
                                                strcpy(sbuf, "drawpad output 1");
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        sbuf, strlen(sbuf) + 1);
                                                strcpy(sbuf, "fpgaRw write 68 3");
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_MCU, COMMAND_BY_CHAR,
                                        sbuf, strlen(sbuf) + 1);
                    }

                    if(A4SGUIData->drawpad.data.en_inter)
                    {
                        char videoName[64] = "HDMI";
                        int i, wincode;
                        for ( i = 0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                        {
                            if(!strcmp(GUIData->product->videoSignalInfo.videoResource[ i].name, videoName))
                            {
                                wincode = i;
                            }
                        }

                                                sprintf(sbuf, "drawpad set %d", wincode);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        sbuf, strlen(sbuf) + 1);

                                                sprintf(sbuf, "lock %d", wincode);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VRM, COMMAND_BY_CHAR,
                                        sbuf, strlen(sbuf) + 1);
                    }

                                        sprintf(sbuf, "drawpad setMask %d", 3);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                    sbuf, strlen(sbuf) + 1);

                                         sprintf(sbuf, "drawpad init");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                    sbuf, strlen(sbuf) + 1);

                    usleep(400000);
                    sprintf(sbuf, "pc gui drawpad open");
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                    sbuf, strlen(sbuf) + 1);
                }
                else if(!strcasecmp( item, "close"))
                {
                    if(A4SGUIData->drawpad.info.winopened == 1)
                    {
                        drawpadMain(GUIData->windows, GUIData->mainWindow, item);

                        if (A4SGUIData->drawpad.data.en_4bit)
                        {
                                                        strcpy(sbuf, "drawpad output 0");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                            sbuf, strlen(sbuf) + 1);

                                                        strcpy(sbuf, "fpgaRw write 68 0");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_MCU, COMMAND_BY_CHAR,
                                            sbuf, strlen(sbuf) + 1);
                        }

                        if (A4SGUIData->drawpad.data.en_inter)
                        {
                            char videoName[64] = "HDMI";
                            int i, wincode;
                            for ( i = 0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                            {
                                if(!strcmp(GUIData->product->videoSignalInfo.videoResource[ i].name, videoName))
                                {
                                    wincode = i;
                                }
                            }

                                                        sprintf(sbuf, "unlock %d", wincode);
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VRM, COMMAND_BY_CHAR,
                                            sbuf, strlen(sbuf) + 1);

                                                        sprintf(sbuf, "drawpad unset %d", wincode);
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                            sbuf, strlen(sbuf) + 1);

                            snprintf(sbuf, sizeof(sbuf)-1, "other restoreSubdrawpad");
                            drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);

                        }

                                                sprintf(sbuf, "drawpad controlmask %d", 0);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        sbuf, strlen(sbuf) + 1);

                                                sprintf(sbuf, "drawpad init");
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        sbuf, strlen(sbuf) + 1);

                        usleep(400000);
                        sprintf(sbuf, "pc gui drawpad close");
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                        sbuf, strlen(sbuf) + 1);
                    }
                }                else if(!strcasecmp(item, "mode"))
                {
                    snprintf(sbuf, sizeof(sbuf)-1, "mode %s", option1);
                    drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
                }
                else if(!strcasecmp(item, "chooseRtsp"))
                {
                    snprintf(sbuf, sizeof(sbuf)-1, "other chooseRtsp %s %s", option1, option2);
                    drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
                }
                else if(!strcasecmp(item, "deChooseRtsp"))
                {
                    snprintf(sbuf, sizeof(sbuf)-1, "other deChooseRtsp");
                    drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
                }
                else if(!strcasecmp(item, "save"))
                {
                    drawpadMain(GUIData->windows, GUIData->mainWindow, "other save");
                }
                else if(!strcasecmp(item, "init"))
                {
                    drawpadMain(GUIData->windows, GUIData->mainWindow, "other init");
                }
                else if(!strcasecmp(item, "setMask"))
                {
                    sprintf(sbuf, "drawpad status %s", option1);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                    sbuf, strlen(sbuf) + 1);
                    snprintf(sbuf, sizeof(sbuf)-1, "other setMask %s", option1);
                    drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
                }
                else if(!strcasecmp(item, "controlmask"))
                {
                    sprintf(sbuf, "drawpad status %s", option1);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                    sbuf, strlen(sbuf) + 1);
                    snprintf(sbuf, sizeof(sbuf)-1, "other controlmask %s", option1);
                    drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
                }
                else if(!strcasecmp(item, "rtspcontrol"))
                {

                    printf("====rtspcontrol=====%s======%s=======\n", option1, option2);

                    switch(A4SGUIData->drawpad.info.mode)
                    {
                        case SINGLE:
                            if(atoi(option1) == 1 || atoi(option1) == 2 || atoi(option1) == 4 )
                            {
                                char msgbuf[64] = "";
                                if(!strcasecmp(option2, "on"))
                                {
                                    sprintf(msgbuf, "drawpad controlmask %d", atoi(option1) * 4);
                                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                                    msgbuf, strlen(msgbuf) + 1);
                                }
                                else if(!strcasecmp(option2, "off"))
                                {
                                    sprintf(msgbuf, "drawpad controlmask %d", 3);
                                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                                    msgbuf, strlen(msgbuf) + 1);
                                }
                            }
                        break;
                        case MULTIPLE:
                            if(atoi(option1) == 1 || atoi(option1) == 2 || atoi(option1) == 4 )
                            {
                                char msgbuf[64] = "";
                                if(!strcasecmp(option2, "on"))
                                {
                                    snprintf(msgbuf, sizeof(msgbuf)-1, "other MultipleModeControl on %s", option1);
                                    drawpadMain(GUIData->windows, GUIData->mainWindow, msgbuf);

                                }
                                else if(!strcasecmp(option2, "off"))
                                {
                                    snprintf(msgbuf, sizeof(msgbuf)-1, "other MultipleModeControl off %s", option1);
                                    drawpadMain(GUIData->windows, GUIData->mainWindow, msgbuf);
                                }
                            }
                        break;
                    }
                }
                else if(!strcasecmp(item, "ctrlppt") )
                {
                    if(!A4SGUIData->drawpad.info.winopened)
                        return 0;

                    if(!strcasecmp(option1, "PPTPAGE"))
                    {
                        snprintf(sbuf, sizeof(sbuf)-1, "other ctrlppt pptpage %s", option2);
                        drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
                    }
                    else if(!strcasecmp(option1, "PPTPAGE_CHANGE"))
                    {
                        memset(sbuf, 0, sizeof(sbuf));
                        snprintf(sbuf, sizeof(sbuf)-1, "other ctrlppt pptpage_change %s", option2);
                        drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
                    }
                    else if(!strcasecmp(option1, "PPTEND"))
                    {
                        drawpadMain(GUIData->windows, GUIData->mainWindow, "other ctrlppt pptend");
                    }
                }
                else if(!strcasecmp(item, "output"))
                {
                    A4SGUIData->DrawPadOutputEnable = atoi(option1);
                }
                else if(!strcasecmp(item, "set"))
                {
                    if(atoi(option1) >= 0 && atoi(option1) < GUIData->product->videoSignalInfo.videoResourceCount)
                        a4s_drawpad_set_data(GUIData, atoi(option1));
                }
                else if(!strcasecmp(item, "unset"))
                {
                    if(atoi(option1) >= 0 && atoi(option1) < GUIData->product->videoSignalInfo.videoResourceCount)
                        a4s_drawpad_unset_data(GUIData, atoi(option1));
                }
                else if(!strcasecmp(item, "unsetall"))
                {
                    int i;
                    for(i=0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                        a4s_drawpad_unset_data(GUIData, i);
                }
                else if(!strcasecmp(item, "color"))
                {
                    if(INTERA_DRAWPAD_MODE == AVA_DRAWPAD_MODE_ARGB1555)
                    {
                        snprintf(sbuf, sizeof(sbuf)-1, "other color 0x%x", ARGBto1555((int)strtoul(option1,0,0)));
                    }
                    else if(INTERA_DRAWPAD_MODE == AVA_DRAWPAD_MODE_ARGB8888)
                    {
                        snprintf(sbuf, sizeof(sbuf)-1, "other color %s", option1);
                    }
                    else
                        return 0;
                    drawpadMain(GUIData->windows, GUIData->mainWindow, sbuf);
                    
                    sprintf(sbuf, "pc gui drawpad color %s", option1);
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                    sbuf, strlen(sbuf) + 1);
                }
                else if(!strcasecmp(item, "lineWidth"))
                {
                    sprintf(sbuf, "pc gui drawpad lineWidth %d", atoi(option1));
                    AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_KMM, COMMAND_BY_CHAR,
                                    sbuf, strlen(sbuf) + 1);
                }
                else if(!strcasecmp(item, "key"))
                {
                    if(A4SGUIData->DrawPadEnable != 1)
                        return 0;

                    if(!strcasecmp(option1, "UP"))
                    {
                        drawpadMain(GUIData->windows, GUIData->mainWindow, "other key up");
                    }
                    else if(!strcasecmp(option1, "DOWN"))
                    {
                        drawpadMain(GUIData->windows, GUIData->mainWindow, "other key down");
                    }
                    else if(!strcasecmp(option1, "TAB"))
                    {
                        char msgbuf[64] = "";
                        if(A4SGUIData->DrawPadTab)
                        {
                            sprintf(msgbuf, "mode %s", A4SGUIData->vgamode);
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf)+1);
                        }
                        else
                        {
                            sprintf(msgbuf, "mode V1");
                            AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                            msgbuf,  strlen( msgbuf)+1);
                        }
                        A4SGUIData->DrawPadTab = !A4SGUIData->DrawPadTab;
                    }
                    else if(!strcasecmp(option1, "ENTER"))
                    {
                    }
                }
                else if(!strcasecmp("print", item))
                {
                    char path[2][64] = {"RTSP1", "HDMI"};
                    int i, j;
                    for(j=0; j<2; j++)                    {
                        for ( i = 0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                        {
                            if(!strcmp(GUIData->product->videoSignalInfo.videoResource[ i].name, path[j]))
                            {
                                printf("%s id=%d\n", path[j], i);
                            }
                        }
                    }

                    for(i=0; i<12; i++)                    {
                        printf("interaVideo[%d]=%s\n", i, A4SGUIData->interaVideo[i]);
                    }
                }
            }            else if(!strcasecmp(command, "subdrawpad"))
            {
                if(!A4SGUIData->DrawPadEnable)
                    return 0;

                char item[64] = "";
                char option1[64] = "";
                char option2[64] = "";
                char msgbuf[512] = "";
                char path[2][64] = {"RTSP1", "HDMI"};
                int i, j, hdminum, rtspnum;

                ava_mtsubs( buf, ' ', 2, item, sizeof( item));
                ava_mtsubs( buf, ' ', 3, option1, sizeof( option1));
                ava_mtsubs( buf, ' ', 4, option2, sizeof( option2));

                                for(j=0; j<2; j++)
                {
                    for ( i = 0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                    {
                        if(!strcmp(GUIData->product->videoSignalInfo.videoResource[ i].name, path[j]))
                        {
                            switch (j)
                            {
                                case 0: rtspnum = i; break;
                                case 1: hdminum = i; break;
                            }
                        }
                    }
                }

                enum subDrawpadCondition{SWITCHHDMI, SWITCHRTSP1, SET, UNSET, COPYLOCK, LOCK, UNLOCK};
                char condition[][64] ={"switchHDMI", "switchRTSP1", "set", "unset", "copylock", "lock", "unlock"};

                for(i=0; i<sizeof(condition)/sizeof(condition[0]); i++)
                {
                    if(!strcasecmp(item, condition[i]))
                        break;
                }

                switch ( (enum subDrawpadCondition)i )
                {
                    case SWITCHHDMI:
                    {
                        sprintf(msgbuf,"modex 15 M1 %s", A4SGUIData->interaVideo[2]);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf, strlen(msgbuf) + 1);
                    }break;
                    case SWITCHRTSP1:
                    {
                        sprintf(msgbuf,"modex 7 M1 %s", A4SGUIData->interaVideo[4]);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VB, COMMAND_BY_CHAR,
                                        msgbuf, strlen(msgbuf) + 1);
                    }break;
                    case SET:
                    {
                        sprintf(msgbuf, "drawpad set %d", hdminum);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        msgbuf, strlen(msgbuf) + 1);
                    }break;
                    case UNSET:
                    {
                        sprintf(msgbuf, "drawpad unset %d", hdminum);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_GUI, COMMAND_BY_CHAR,
                                        msgbuf, strlen(msgbuf) + 1);
                    }break;
                    case COPYLOCK:
                    {
                        sprintf(msgbuf, "copylock %d %d", rtspnum, hdminum);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VRM, COMMAND_BY_CHAR,
                                        msgbuf, strlen(msgbuf) + 1);
                    }break;
                    case LOCK:
                    {
                        sprintf(msgbuf, "lock %d", hdminum);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VRM, COMMAND_BY_CHAR,
                                        msgbuf, strlen(msgbuf) + 1);
                    }break;
                    case UNLOCK:
                    {
                        sprintf(msgbuf, "unlock %d", hdminum);
                        AVA_MessagePost(GUIData->globalMessage, MODULE_GUI, MODULE_VRM, COMMAND_BY_CHAR,
                                        msgbuf, strlen(msgbuf) + 1);
                    }break;
                }
            }            else if (!strcasecmp(command, "getstate"))
            {
                char item[64] = "";

                ava_mtsubs( buf, ' ', 2, item, sizeof( item));
                if ( !strcasecmp( item, "intera"))
                {
                    if(strlen(A4SGUIData->interaSignal) > 0)
                        return 1;
                    else
                        return 0;
                }
            }
            else if ( !strcasecmp( command, "rmmamsg"))
            {
                if(A4SGUIData->interaProxySwitch != 2)
                    return -1;
                char sbuf[64] = "";
                int dst = atoi(ava_mtsubs( buf, ' ', 2, sbuf, sizeof(sbuf)));

                gui_Rmma_Sendmsg(GUIData, dst, buf+strlen(command)+1+strlen(sbuf)+1);
            }
            else if (!strcasecmp(command, "currentLanguage"))
            {
                char sbuf[64] = "";

                if(!strcasecmp(GUIData->devName, "sv06"))
                    strcpy(sbuf, "SIMPLIFIEDCHINESE");
                else
                    ava_mtsubs( buf, ' ', 2, sbuf, sizeof( sbuf));
                if(!strcasecmp(sbuf, "ENGLISH") || !strcasecmp(sbuf, "E"))
                    GUIData->currentLanguage = LANGUAGE_ENGLISH;
                else if(!strcasecmp(sbuf, "SIMPLIFIEDCHINESE") || !strcasecmp(sbuf, "S"))
                    GUIData->currentLanguage = LANGUAGE_SIMPLIFIEDCHINESE;
                else if(!strcasecmp(sbuf, "TRIDITIONALCHINESE") || !strcasecmp(sbuf, "T"))
                    GUIData->currentLanguage = LANGUAGE_TRIDITIONALCHINESE;
                else
                {
                    GUIData->currentLanguage = LANGUAGE_SIMPLIFIEDCHINESE;
                    strcpy(sbuf, "SIMPLIFIEDCHINESE");
                }
                SetConfigData(GUIData, "gui", "currentLanguage", sbuf);
                sprintf(sbuf, "%d", MAINWINDOW_LANGUAGECHANGE);
                SendCommandToWindow(GUIData->windows, GUIData->mainWindow, CONTROL_WINDOW_OTHER, sbuf);
            }
            else if (!strcasecmp(command, "deleteFile"))
            {
                        char param[64] = "";
                        char option1[64] = "";

                ava_mtsubs( buf, ' ', 2, param, sizeof( param));
                ava_mtsubs( buf, ' ', 3, option1, sizeof( option1));
                if ( !strcasecmp( param, "lessthan"))
                {
                    gui_delFile_by_size(GUIData->windows, GUIData->mainWindow, 0, atoi(option1));
                }
                else if ( !strcasecmp( param, "morethan"))
                {
                    gui_delFile_by_size(GUIData->windows, GUIData->mainWindow, 1, atoi(option1));
                }
            }
            else if (!strcasecmp(command, "setudiskdata"))
            {
                        int i;
                char cmd2[256] = "";
                char name[64] = "";
                char path[64] = "";
                char node[64] = "";
                char sbuf[64] = "";
                char udiskname[128] = "";
                char udiskpath[128] = "";
                char udisknode[128] = "";

                strcpy(cmd2, buffer + strlen( command) + 1);

                i=1;
                while(strlen(ava_mtsubs( cmd2, ',', i, sbuf, sizeof( sbuf))) > 2)
                {
                    if(strlen(ava_mtsubs( sbuf, '=', 1, name, sizeof( name))) > 0 && strlen(ava_mtsubs( sbuf, '=', 2, path, sizeof( path))) > 0)
                    {
                        ava_mtsubs( sbuf, '=', 3, node, sizeof( node));
                        if(i == 1)
                        {
                            strcpy(udiskname, name);
                            strcpy(udiskpath, path);
                            strcpy(udisknode, node);
                        }
                        else
                        {
                            sprintf(udiskname, "%s\t%s", udiskname, name);
                            sprintf(udiskpath, "%s\t%s", udiskpath, path);
                            sprintf(udisknode, "%s\t%s", udisknode, node);
                        }
                    }
                    i++;
                }
                strcpy(A4SGUIData->udiskname, udiskname);
                strcpy(A4SGUIData->udiskpath, udiskpath);
                strcpy(A4SGUIData->udisknode, udisknode);

                sprintf(sbuf, "%d", REFRESH_UDISK);
                SendCommandToWindow(GUIData->windows, &RecSetup, CONTROL_WINDOW_OTHER, sbuf);
                SendCommandToWindow(GUIData->windows, &FileDownload, CONTROL_WINDOW_OTHER, sbuf);
                SendCommandToWindow(GUIData->windows, &FileSave, CONTROL_WINDOW_OTHER, sbuf);
                SendCommandToWindow(GUIData->windows, &LicenseDownload, CONTROL_WINDOW_OTHER, sbuf);
            }
            else if (!strcasecmp(command, "udiskselect"))
            {
                strcpy(A4SGUIData->udiskselect, buffer + strlen( command) + 1);
            }
            else if (!strcasecmp(command, "setcdromdata"))
            {
                int i;
                char cmd2[256] = "";
                char name[64] = "";
                char path[64] = "";
                char sbuf[64] = "";
                char cdromname[128] = "";
                char cdrompath[128] = "";

                strcpy(cmd2, buffer + strlen( command) + 1);

                i=1;
                while(strlen(ava_mtsubs( cmd2, ',', i, sbuf, sizeof( sbuf))) > 2)
                {
                    if(strlen(ava_mtsubs( sbuf, '=', 1, name, sizeof( name))) > 0 && strlen(ava_mtsubs( sbuf, '=', 2, path, sizeof( path))) > 0)
                    {
                        if(i == 1)
                        {
                            strcpy(cdromname, name);
                            strcpy(cdrompath, path);
                        }
                        else
                        {
                            sprintf(cdromname, "%s\t%s", cdromname, name);
                            sprintf(cdrompath, "%s\t%s", cdrompath, path);
                        }
                    }
                    i++;
                }
                strcpy(A4SGUIData->cdromname, cdromname);
                strcpy(A4SGUIData->cdrompath, cdrompath);

                sprintf(sbuf, "%d", REFRESH_CDROM);
                SendCommandToWindow(GUIData->windows, &RecSetup, CONTROL_WINDOW_OTHER, sbuf);
                SendCommandToWindow(GUIData->windows, &sys_cdformat, CONTROL_WINDOW_OTHER, sbuf);
            }
            else if (!strcasecmp(command, "cdromselect"))
            {
                strcpy(A4SGUIData->cdromselect, buffer + strlen( command) + 1);
            }
            else if (!strcasecmp(command, "LOGOUPLOAD"))
            {
                int ret;
                char sbuf[64] = "";
                char sbuf2[64] = "";
                char src[64] = "";
                char dst[64] = "";

                ava_mtsubs( buf, ' ', 2, src, sizeof( src));
                if(strlen(src)>0)
                {
                    strcpy(dst, GUIData->osd[0].url);
                    if(access(src, F_OK) >= 0)
                    {
                        unsigned int pVideoLogoWidth, pVideoLogoHeight, bpp;
                        AVA_GetBMPInfo( src, &pVideoLogoWidth, &pVideoLogoHeight, &bpp);
                        if(pVideoLogoWidth > 300 || pVideoLogoHeight > 300)
                        {
                            getLanguageString( OSDLOGOSETUP_OSDLOGOINFOSAVE_TIPS3, GUIData->currentLanguage, sbuf);
                            if(srcId == MODULE_HTTP)
                            {
                                sprintf(sbuf2, "pc ct status %s", sbuf);
                                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                                sbuf2,  strlen( sbuf2) + 1 );
                            }
                            else if(srcId == MODULE_GUI)
                            {
                                gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
                            }
                            else if(srcId == MODULE_TRM)
                            {
                                printf("%s %s\n", src, sbuf);
                            }
                            return 0;
                        }

                        ret = ava_file_copy(src, dst);
                        if(ret == 0)
                        {
                            getLanguageString( OSDLOGOSETUP_OSDLOGOINFOSAVE_TIPS1, GUIData->currentLanguage, sbuf);
                            A4SGUIData->LogoWidth = pVideoLogoWidth;
                            A4SGUIData->LogoHeight = pVideoLogoHeight;
                            sprintf(sbuf2, "pc osd logosize %d %d", A4SGUIData->LogoWidth, A4SGUIData->LogoHeight);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                            sbuf2,  strlen( sbuf2) + 1 );
                        }
                        else
                            getLanguageString( OSDLOGOSETUP_OSDLOGOINFOSAVE_TIPS2, GUIData->currentLanguage, sbuf);

                        if(srcId == MODULE_HTTP)
                        {
                            sprintf(sbuf2, "pc ct status %s", sbuf);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                            sbuf2,  strlen( sbuf2) + 1 );
                        }
                        else if(srcId == MODULE_GUI)
                        {
                            gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
                        }
                        else if(srcId == MODULE_TRM)
                        {
                            printf("%s %s\n", src, sbuf);
                        }
                    }
                    else
                    {
                        getLanguageString( OSDLOGOSETUP_OSDLOGOINFOSAVE_TIPS4, GUIData->currentLanguage, sbuf);
                        if(srcId == MODULE_HTTP)
                        {
                            sprintf(sbuf2, "pc ct status %s", sbuf);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                            sbuf2,  strlen( sbuf2) + 1 );
                        }
                        else if(srcId == MODULE_GUI)
                        {
                            gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
                        }
                        else if(srcId == MODULE_TRM)
                        {
                            printf("%s %s\n", src, sbuf);
                        }
                    }
                }
            }
            else if (!strcasecmp(command, "FILEUPLOAD"))
            {
                int i, flag;
                char filename[64];
                char sbuf[64];
                char sbuf2[64];
                char src[64];
                char dst[64];
                char type[64];

                ava_mtsubs( buf, ' ', 2, type, sizeof( type));
                ava_mtsubs( buf, ' ', 3, filename, sizeof( filename));

                flag = 0;
                for ( i = 0; i<GUIData->product->videoSignalInfo.videoResourceCount; i++)
                {
                    if(!strcmp(GUIData->product->videoSignalInfo.videoResource[ i].name, type))
                    {
                        flag = 1;
                        break;
                    }
                }
                if(flag == 1)
                {
                    strcpy(src, filename);
                    strcpy(dst, GUIData->product->videoSignalInfo.videoResource[ i].url);
                    VideoSignal *vr = &GUIData->product->videoSignalInfo.videoResource[ i];

                    if((access(src, F_OK))==-1)
                    {
                        getLanguageString( IMAGESETUP_HANDLER_TIPS1, GUIData->currentLanguage, sbuf);
                        if(srcId == MODULE_HTTP)
                        {
                            sprintf(sbuf2, "pc ct status %s", sbuf);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                            sbuf2,  strlen( sbuf2) + 1 );
                        }
                        else if(srcId == MODULE_GUI)
                        {
                            gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
                        }
                        else if(srcId == MODULE_TRM)
                        {
                            printf("%s %s\n", src, sbuf);
                        }
                        return 0;                     }
                    strcpy(type, src + strlen(src)-3);
                    if ( !strcasecmp( type, "JPG"))
                    {
                        if ( vr->VbBlk !=-1)
                        {
                            HI_MPI_VB_ReleaseBlock(vr->VbBlk);
                            vr->VbBlk = -1;
                        }
                        if ( vr->VbBlk_s !=-1)
                        {
                            HI_MPI_VB_ReleaseBlock(vr->VbBlk_s);
                            vr->VbBlk_s = -1;
                        }
                        if(AVA_LoadJPEG( src, vr->chnID, &vr->VbBlk, &vr->buffer, &vr->VbBlk_s, &vr->buffer_s))
                        {
                            getLanguageString( IMAGESETUP_HANDLER_TIPS2, GUIData->currentLanguage, sbuf);
                            if(srcId == MODULE_HTTP)
                            {
                                sprintf(sbuf2, "pc ct status %s", sbuf);
                                AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                                sbuf2,  strlen( sbuf2) + 1 );
                            }
                            else if(srcId == MODULE_GUI)
                            {
                                gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
                            }
                            else if(srcId == MODULE_TRM)
                            {
                                printf("%s %s\n", src, sbuf);
                            }
                            return 0;
                        }
                    }
                    else
                    {
                        getLanguageString( IMAGESETUP_HANDLER_TIPS2, GUIData->currentLanguage, sbuf);
                        if(srcId == MODULE_HTTP)
                        {
                            sprintf(sbuf2, "pc ct status %s", sbuf);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                            sbuf2,  strlen( sbuf2) + 1 );
                        }
                        else if(srcId == MODULE_GUI)
                        {
                            gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
                        }
                        else if(srcId == MODULE_TRM)
                        {
                            printf("%s %s\n", src, sbuf);
                        }
                        return 0;
                    }
                    getLanguageString( IMAGESETUP_HANDLER_TIPS3, GUIData->currentLanguage, sbuf);

                    if((access(src, F_OK)) < 0)
                    {
                        getLanguageString( IMAGESETUP_HANDLER_TIPS5, GUIData->currentLanguage, sbuf);
                        if(srcId == MODULE_HTTP)
                        {
                            sprintf(sbuf2, "pc ct status %s", sbuf);
                            AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                            sbuf2,  strlen( sbuf2) + 1 );
                        }
                        else if(srcId == MODULE_GUI)
                        {
                            gui_Tips_display(GUIData, TIPS_OPEN, sbuf);
                        }
                        else if(srcId == MODULE_TRM)
                        {
                            printf("%s %s\n", src, sbuf);
                        }
                        return 0;                     }

                    usleep(1000);
                    if(srcId == MODULE_HTTP)
                    {
                    }
                    else if(srcId == MODULE_GUI)
                    {
                        gui_Tips_display(GUIData, TIPS_LOCK, sbuf);
                    }
                    else if(srcId == MODULE_TRM)
                    {
                        printf("%s %s\n", src, sbuf);
                    }

                    flag = ava_file_copy(src, dst);
                    if(flag == 0)
                        getLanguageString( IMAGESETUP_HANDLER_TIPS6, GUIData->currentLanguage, sbuf);
                    else
                        getLanguageString( IMAGESETUP_HANDLER_TIPS8, GUIData->currentLanguage, sbuf);

                    if(srcId == MODULE_HTTP)
                    {
                        sprintf(sbuf2, "pc ct status %s", sbuf);
                        AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_HTTP,  COMMAND_BY_CHAR,
                                        sbuf2,  strlen( sbuf2) + 1 );
                    }
                    else if(srcId == MODULE_GUI)
                    {
                        gui_Tips_display(GUIData, TIPS_UNLOCK, sbuf);
                    }
                    else if(srcId == MODULE_TRM)
                    {
                        printf("%s %s\n", src, sbuf);
                    }
                    ava_mtsubs( buf, ' ', 2, type, sizeof( type));
                    sprintf(sbuf2, "resstart %s", type);
                    AVA_MessagePost( GUIData->globalMessage,  MODULE_GUI, MODULE_VRM,  COMMAND_BY_CHAR,
                                    sbuf2,  strlen( sbuf2) + 1 );
                }
            }
            break;
        default:
            break;
        }
    }

    return 0;
}


