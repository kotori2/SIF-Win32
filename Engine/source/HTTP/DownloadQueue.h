namespace MicroDownload
{
	void Queue(const char* callback, const char* file_target, const char* download_url);
	void DeleteAll();
	void MainLoop(int deltaT);
}
