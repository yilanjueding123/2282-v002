#include "video_encoder.h"

void video_encode_entrance(void)
{
    INT32S nRet;

    avi_encode_init();
    nRet = avi_encode_state_task_create(AVI_ENC_PRIORITY);
    if(nRet < 0)
        DBG_PRINT("avi_encode_state_task_create fail !!!\r\n");

    nRet = scaler_task_create(SCALER_PRIORITY);
    if(nRet < 0)
        DBG_PRINT("scaler_task_create fail !!!\r\n");

    nRet = video_encode_task_create(JPEG_ENC_PRIORITY);
    if(nRet < 0)
        DBG_PRINT("video_encode_task_create fail !!!\r\n");

    nRet = avi_adc_record_task_create(AUD_ENC_PRIORITY);
    if(nRet < 0)
        DBG_PRINT("avi_adc_record_task_create fail !!!\r\n");

    DBG_PRINT("avi encode all task create success!!!\r\n");
}

void video_encode_exit(void)
{
    INT32S nRet;

    if(video_encode_status() == VIDEO_CODEC_PROCESSING)
        video_encode_stop();

    video_encode_preview_stop();

    nRet = avi_encode_state_task_del();
    if(nRet < 0)
        DBG_PRINT("avi_encode_state_task_del fail !!!");

    nRet = scaler_task_del();
    if(nRet < 0)
        DBG_PRINT("scaler_task_del fail !!!");

    nRet = video_encode_task_del();
    if(nRet < 0)
        DBG_PRINT("video_encode_task_del fail !!!");

    nRet = avi_adc_record_task_del();
    if(nRet < 0)
        DBG_PRINT("avi_adc_record_task_del fail !!!");

    DBG_PRINT("avi encode all task delete success!!!\r\n");
}

CODEC_START_STATUS video_encode_preview_start(VIDEO_ARGUMENT arg)
{
    INT32S nRet;

    pAviEncAudPara->audio_format = AVI_ENCODE_AUDIO_FORMAT;
    pAviEncAudPara->channel_no = 1; //mono
    pAviEncAudPara->audio_sample_rate = arg.AudSampleRate;

    pAviEncVidPara->video_format = AVI_ENCODE_VIDEO_FORMAT;
    pAviEncVidPara->dwScale = arg.bScaler;
    pAviEncVidPara->dwRate = arg.VidFrameRate;

    avi_encode_set_display_format(arg.OutputFormat);
    avi_encode_set_sensor_format(BITMAP_YUYV);

    pAviEncVidPara->sensor_capture_width = arg.SensorWidth;
    pAviEncVidPara->sensor_capture_height = arg.SensorHeight;
    pAviEncVidPara->encode_width = arg.TargetWidth;
    pAviEncVidPara->encode_height = arg.TargetHeight;
    pAviEncVidPara->display_width = arg.DisplayWidth;
    pAviEncVidPara->display_height = arg.DisplayHeight;

    if(arg.DisplayBufferWidth == 0)
        arg.DisplayBufferWidth = arg.DisplayWidth;
    else if(arg.DisplayWidth > arg.DisplayBufferWidth)
        arg.DisplayWidth = arg.DisplayBufferWidth;

    if(arg.DisplayBufferHeight == 0)
        arg.DisplayBufferHeight = arg.DisplayHeight;
    else if(arg.DisplayHeight > arg.DisplayBufferHeight)
        arg.DisplayHeight = arg.DisplayBufferHeight;

    pAviEncVidPara->display_buffer_width = arg.DisplayBufferWidth;
    pAviEncVidPara->display_buffer_height = arg.DisplayBufferHeight;
    avi_encode_set_display_scaler();
    DBG_PRINT("fklll\r\n");
    nRet = vid_enc_preview_start();
    DBG_PRINT("fkbbbb\r\n");
    if(nRet < 0)
        return CODEC_START_STATUS_ERROR_MAX;

    return START_OK;
}

CODEC_START_STATUS video_encode_preview_stop(void)
{
    INT32S result;

    result = vid_enc_preview_stop();
    if(result < 0)
        return CODEC_START_STATUS_ERROR_MAX;

    return START_OK;
}

CODEC_START_STATUS video_encode_start(MEDIA_SOURCE src)
{
    INT32S nRet;

    if(src.type == SOURCE_TYPE_FS)
        pAviEncPara->source_type = SOURCE_TYPE_FS;
    else if(src.type == SOURCE_TYPE_USER_DEFINE)
        pAviEncPara->source_type = SOURCE_TYPE_USER_DEFINE;
    else
        return RESOURCE_WRITE_ERROR;

    if(src.type_ID.FileHandle < 0)
        return RESOURCE_NO_FOUND_ERROR;

    if(src.Format.VideoFormat == MJPEG)
        pAviEncVidPara->video_format = C_MJPG_FORMAT;
#if	MPEG4_ENCODE_ENABLE == 1
    else if(src.Format.VideoFormat == MPEG4)
        pAviEncVidPara->video_format = C_XVID_FORMAT;
#endif
    else
        return RESOURCE_WRITE_ERROR;

    avi_encode_set_curworkmem((void *)pAviEncPacker0);
    nRet = avi_encode_set_file_handle_and_caculate_free_size(pAviEncPara->AviPackerCur, src.type_ID.FileHandle);
    if(nRet < 0)
        return RESOURCE_WRITE_ERROR;

    //start avi packer
    nRet = avi_enc_packer_start(pAviEncPara->AviPackerCur);
    if(nRet < 0)
        return CODEC_START_STATUS_ERROR_MAX;

    //start avi encode
    nRet = avi_enc_start();
    if(nRet < 0)
        return CODEC_START_STATUS_ERROR_MAX;

    return START_OK;
}

CODEC_START_STATUS video_encode_stop(void)
{
    INT32S nRet, nTemp;

    if(avi_encode_get_status() & C_AVI_ENCODE_PAUSE)
        nRet = avi_enc_resume();

    //stop avi encode
    nRet = avi_enc_stop();
    //stop avi packer
    nTemp = avi_enc_packer_stop(pAviEncPara->AviPackerCur);

    if(nRet < 0 || nTemp < 0)
        return CODEC_START_STATUS_ERROR_MAX;

    return START_OK;
}

CODEC_START_STATUS video_encode_Info(VIDEO_INFO *info)
{
    switch(pAviEncAudPara->audio_format)
    {
    case WAVE_FORMAT_PCM:
        info->AudFormat = WAV;
        gp_strcpy((INT8S *)info->AudSubFormat, (INT8S *)"pcm");
        info->AudBitRate = pAviEncAudPara->audio_sample_rate * 16;
        break;

    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
        info->AudFormat = WAV;
        gp_strcpy((INT8S *)info->AudSubFormat, (INT8S *)"adpcm");
        info->AudBitRate = pAviEncAudPara->audio_sample_rate * 16 / 4;
        break;

    case WAVE_FORMAT_ADPCM:
        info->AudFormat = MICROSOFT_ADPCM;
        gp_strcpy((INT8S *)info->AudSubFormat, (INT8S *)"adpcm");
        info->AudBitRate = pAviEncAudPara->audio_sample_rate * 16 / 4;
        break;

    case WAVE_FORMAT_IMA_ADPCM:
        info->AudFormat = IMA_ADPCM;
        gp_strcpy((INT8S *)info->AudSubFormat, (INT8S *)"adpcm");
        info->AudBitRate = pAviEncAudPara->audio_sample_rate * 16 / 4;
        break;

    default:
        while(1);
    }
    info->AudSampleRate = pAviEncAudPara->audio_sample_rate;
    info->AudChannel = 1;//mono

    switch(pAviEncVidPara->video_format)
    {
    case C_MJPG_FORMAT:
        info->VidFormat = MJPEG;
        gp_strcpy((INT8S *)info->VidSubFormat, (INT8S *)"jpg");
        break;

    case C_XVID_FORMAT:
        info->VidFormat = MPEG4;
        gp_strcpy((INT8S *)info->VidSubFormat, (INT8S *)"mp4");
        break;

    default:
        while(1);
    }

    info->VidFrameRate = pAviEncVidPara->dwRate;
    info->Width = pAviEncVidPara->encode_width;
    info->Height = pAviEncVidPara->encode_height;
    return	START_OK;
}

VIDEO_CODEC_STATUS video_encode_status(void)
{
    INT32U status;

    status = avi_encode_get_status();
    if(status & C_AVI_ENCODE_PAUSE)
        return VIDEO_CODEC_PROCESS_PAUSE;

    if(status & C_AVI_ENCODE_START)
        return VIDEO_CODEC_PROCESSING;

    return VIDEO_CODEC_PROCESS_END;
}

CODEC_START_STATUS video_encode_auto_switch_csi_frame(void)
{
    avi_encode_switch_csi_frame_buffer();
    return START_OK;
}

CODEC_START_STATUS video_encode_auto_switch_csi_fifo_end(INT8U flag)
{
    vid_enc_csi_fifo_end(flag, 0);
    return START_OK;
}

CODEC_START_STATUS video_encode_auto_switch_csi_frame_end(INT8U flag)
{
    vid_enc_csi_fifo_end(flag, 1);
    return START_OK;
}

CODEC_START_STATUS video_encode_set_zoom_scaler(FP32 zoom_ratio)
{
    pAviEncVidPara->scaler_zoom_ratio = zoom_ratio;
    return START_OK;
}

CODEC_START_STATUS video_encode_pause(void)
{
    INT32S nRet;

    nRet = avi_enc_pause();
    if(nRet < 0)
        return CODEC_START_STATUS_ERROR_MAX;

    return START_OK;
}

CODEC_START_STATUS video_encode_resume(void)
{
    INT32S nRet;

    nRet = avi_enc_resume();
    if(nRet < 0)
        return CODEC_START_STATUS_ERROR_MAX;

    return START_OK;
}

CODEC_START_STATUS video_encode_capture_picture(MEDIA_SOURCE src)
{
    //	INT16S disk_no;
    //	INT32S size;

    if(src.type_ID.FileHandle < 0)
        return CODEC_START_STATUS_ERROR_MAX;

    pAviEncPara->AviPackerCur->file_handle = src.type_ID.FileHandle;
    /*	size = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height;
    	disk_no = GetDiskOfFile(src.type_ID.FileHandle);
    	if(vfsFreeSpace(disk_no) < size)
    	{
    		close(src.type_ID.FileHandle);
    		return CODEC_START_STATUS_ERROR_MAX;
    	}*/

    if(avi_enc_save_jpeg() < 0)
    {
        close(src.type_ID.FileHandle);
        return CODEC_START_STATUS_ERROR_MAX;
    }

    return START_OK;
}

/*
CODEC_START_STATUS video_encode_fast_switch_stop_and_start(MEDIA_SOURCE src)
{
	INT32S nRet;

   	DBG_PRINT("=>video_encode_fast_stop_and_start!!!\r\n");

    if(src.type == SOURCE_TYPE_FS)
    	pAviEncPara->source_type = SOURCE_TYPE_FS;
    else if(src.type == SOURCE_TYPE_USER_DEFINE)
    	pAviEncPara->source_type = SOURCE_TYPE_USER_DEFINE;
    else
        return RESOURCE_WRITE_ERROR;

    if(src.type_ID.FileHandle < 0)
        return RESOURCE_NO_FOUND_ERROR;

    nRet = avi_encode_set_file_handle_and_caculate_free_size(pAviEncPacker0, src.type_ID.FileHandle);
	if(nRet < 0) return RESOURCE_WRITE_ERROR;

	nRet = avi_enc_packer_start(pAviEncPacker0);
	if(nRet < 0) {
    	return CODEC_START_STATUS_ERROR_MAX;
    }

    return START_OK;
}
*/
