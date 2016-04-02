#include "AwDataSource.h"
#include "Core/Stream/FileStream.h"

void AwDataSource::OnRequest (int id, const Awesomium::WebString &path)
{
	char temp [512];
	path.ToUTF8 (temp, sizeof (temp));
	String url = temp;

	FileStream stream;
	if (!stream.open (url, Torque::FS::File::Read))
	{
		SendResponse (id, 0, 0, Awesomium::WebString ());
		return;
	}

	U8 *buffer = new U8 [stream.getStreamSize ()];
	stream.read (stream.getStreamSize (), buffer);
	String mime = "text/html";
	SendResponse (id, stream.getStreamSize (), buffer, Awesomium::WebString::CreateFromUTF8 (mime, mime.length ()));
	delete buffer;
	stream.close ();
}