#include "InputStream.hpp"

extern "C" {
#ifndef __STDC_CONSTANT_MACROS
    #define __STDC_CONSTANT_MACROS
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/avstring.h>
}

#include <stdexcept>
#include <cassert>

namespace avtranscoder
{

InputStream::InputStream( const std::string& filename, const size_t streamIndex )
		: m_formatContext( NULL )
		, m_streamIndex( streamIndex )
{
	init( filename );
};

InputStream::~InputStream( )
{
	if( m_formatContext != NULL )
	{
		avformat_close_input( &m_formatContext );
		m_formatContext = NULL;
	}
}

bool InputStream::readNextPacket( DataStream& data ) const
{
	AVPacket packet;
	av_init_packet( &packet );

	readNextPacket( packet );

	// is it possible to remove this copy ?
	// using : av_packet_unref ?
	data.getBuffer().resize( packet.size );
	memcpy( data.getPtr(), packet.data, packet.size );

	av_free_packet( &packet );

	return true;
}

bool InputStream::readNextPacket( AVPacket& packet ) const
{
	while( 1 )
	{
		int ret = av_read_frame( m_formatContext, &packet );
		if( ret < 0 ) // error or end of file
		{
			return false;
		}

		// We only read one stream and skip others
		if( packet.stream_index == (int)m_streamIndex )
		{
			return true;
		}
	}
}

VideoDesc InputStream::getVideoDesc() const
{
	std::cout << "get video desc on " << m_formatContext << " / " << m_streamIndex << std::endl;
	assert( m_formatContext != NULL );
	assert( m_streamIndex <= m_formatContext->nb_streams );

	if( m_formatContext->streams[m_streamIndex]->codec->codec_type != AVMEDIA_TYPE_VIDEO )
	{
		return VideoDesc( AV_CODEC_ID_NONE );
	}

	AVCodecContext* codecContext = m_formatContext->streams[m_streamIndex]->codec;

	std::cout << "get video desc with codec id " << codecContext << std::endl;

	VideoDesc desc( codecContext->codec_id );

	desc.setImageParameters( codecContext->width, codecContext->height, codecContext->pix_fmt );
	desc.setTimeBase( codecContext->time_base.num, codecContext->time_base.den );

	return desc;
}

AudioDesc InputStream::getAudioDesc() const
{
	assert( m_streamIndex <= m_formatContext->nb_streams );
	if( m_formatContext->streams[m_streamIndex]->codec->codec_type != AVMEDIA_TYPE_AUDIO )
	{
		return AudioDesc( AV_CODEC_ID_NONE );
	}

	AVCodecContext* codecContext = m_formatContext->streams[m_streamIndex]->codec;

	AudioDesc desc( codecContext->codec_id );

	return desc;
}

void InputStream::init( const std::string& filename )
{
	av_register_all();  // Warning: should be called only once
	if( avformat_open_input( &m_formatContext, filename.c_str(), NULL, NULL ) < 0 )
	{
		throw std::runtime_error( "unable to open file" );
	}

	// update format context informations from streams
	if( avformat_find_stream_info( m_formatContext, NULL ) < 0 )
	{
		avformat_close_input( &m_formatContext );
		m_formatContext = NULL;
		throw std::runtime_error( "unable to find stream informations" );
	}
	assert( m_formatContext != NULL );
}

}