#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>



static int open_input_file(AVFormatContext **ifmt_ctx, const char *filename)
{
	int ret = 0;
	AVFormatContext *fmt_ctx = NULL;
	
    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }
	
    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    av_dump_format(fmt_ctx, 0, filename, 0);
	*ifmt_ctx = fmt_ctx;
	
	return 0;
}

static int open_output_file(AVFormatContext *ifmt_ctx, AVFormatContext **ofmt_ctx, const char *filename)
{
	int i = 0, ret = 0;
	AVFormatContext *fmt_ctx = NULL;
	AVStream *in_stream = NULL;
	AVStream *out_stream = NULL;
	
    avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename);
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		in_stream = ifmt_ctx->streams[i];
		out_stream = avformat_new_stream(fmt_ctx, NULL);
		
		ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n", i);
			return ret;
		}
		out_stream->time_base = in_stream->time_base;
	}
	
	av_dump_format(fmt_ctx, 0, filename, 1);

    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(fmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }

	*ofmt_ctx = fmt_ctx;
	
	return 0;
}




/*************************************************
Function:		main
Description:	入口点函数
*************************************************/
int main(int argc, char **argv)
{

	char input_name[32];
	char output_name[32];

	AVOutputFormat *ofmt = NULL;
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	int ret = 0;

	if (argc != 3) {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }

	strcpy(input_name, argv[1]);	//"sample_cif.h264");
	strcpy(output_name, argv[2]);	//"out.avi");
	
	av_register_all();

#if 0
	//按封装格式打开输入视频文件
	if ((ret = avformat_open_input(&ifmt_ctx, input_name, NULL, NULL)) < 0)
	{
		printf("Error: Open input file failed.\n");
		goto end;
	}

	//获取输入视频文件中的流信息
	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
	{
		printf("Error: Failed to retrieve input stream information.\n");
		goto end;
	}
	av_dump_format(ifmt_ctx, 0, input_name, 0);
#else
	open_input_file(&ifmt_ctx, input_name);
#endif


#if 0
	//按照文件名获取输出文件的句柄
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_name);
	if (!ofmt_ctx)
	{
		printf("Error: Could not create output context.\n");
		goto end;
	}
	ofmt = ofmt_ctx->oformat;

	for (unsigned int i = 0; i < ifmt_ctx->nb_streams ; i++)
	{
		AVStream *inStream = ifmt_ctx->streams[i];
		AVStream *outStream = avformat_new_stream(ofmt_ctx, inStream->codec->codec);
		if (!outStream)
		{
			printf("Error: Could not allocate output stream.\n");
			goto end;
		}

		ret = avcodec_copy_context(outStream->codec, inStream->codec);
		outStream->codec->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		{
			outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	av_dump_format(ofmt_ctx, 0, output_name, 1);
	
	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&ofmt_ctx->pb, output_name, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("Error: Could not open output file.\n");
			goto end;
		}
	}

	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) 
	{
		printf("Error: Could not write output file header.\n");
		goto end;
	}
#else
	open_output_file(ifmt_ctx, &ofmt_ctx, output_name);
	ofmt = ofmt_ctx->oformat;
#endif

	while (1) 
	{
		AVStream *in_stream, *out_stream;

		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;

		in_stream  = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		
		/* copy packet */
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;

		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0) 
		{
			fprintf(stderr, "Error muxing packet\n");
			break;
		}
		av_packet_unref(&pkt);
	}

	av_write_trailer(ofmt_ctx);

end:
	av_packet_unref(&pkt);
	avformat_close_input(&ifmt_ctx);

	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) {
		avio_closep(&ofmt_ctx->pb);
	}

	avformat_free_context(ofmt_ctx);

	if (ret < 0 && ret != AVERROR_EOF) 
	{
		fprintf(stderr, "Error failed to write packet to output file.\n");
		return 1;
	}
	return 0;
}
