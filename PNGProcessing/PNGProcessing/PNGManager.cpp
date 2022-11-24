#include "PNGManager.h"
using namespace std;
using namespace filesystem;

PNGManager::PNGManager(const path from, const path to) : from(from), to(to)
{
	if (!is_directory(from) || !is_directory(to)) {
		cerr << "Error: Path Error." << "\n";
		return;
	}

	// collect png paths
	for (auto const& p : directory_iterator{ from }) {
		if (is_png(p)) {
			pngFilePaths.emplace_back(p);
		}
	}
}

void PNGManager::RunBlur()
{
	for (const path& p : pngFilePaths) {
		cout << "//////////////////////////////////////////////////////////////\n"
			<< "Processing " << p.string() << "\n";

		// Read File
		PNGBuffer in{ p };
		if (!in.state) continue;

		// 1.DownSample
		// 2.GaussionBlur
		// https://www.intel.com/content/www/us/en/developer/articles/technical/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html
		
		// Create Buffer
		PNGBuffer half{ in.width / 2, in.height / 2 };
		PNGBuffer mid{ in.width / 2, in.height / 2 }; // tempory, half --horizontal--> mid --vertial--> out
		PNGBuffer out{ in.width / 2, in.height / 2 };

		// Multi Threads
		multi_threads_working<DownSample2x>(in, half);
		multi_threads_working<GaussionBlur1st>(half, mid); // 2-pass for sync
		multi_threads_working<GaussionBlur2nd>(mid, out);

		// Write File
		static const string adder = "_blur";
		out.WriteBufferToFile(path{ to.string() + rename(p.filename(), adder) });

		cout << "Finished.\n";
	}
}

void PNGManager::RunEdge()
{
	for (const path& p : pngFilePaths) {
		cout << "//////////////////////////////////////////////////////////////\n"
			<< "Processing " << p.string() << "\n";

		// Read File
		PNGBuffer in{ p };
		if (!in.state) continue;

		// 1.DownSample
		// 2.GaussionBlur
		// 3.GrayScale
		// 4.EdgeDetection
		// 5.UpSample

		// Create Buffer
		unsigned width_half = in.width / 2;
		unsigned height_half = in.height / 2;
		unsigned width_down4x = in.width / 4;
		unsigned height_down4x = in.height / 4;
		PNGBuffer half{ width_half, height_half };
		PNGBuffer down4x{ width_down4x, height_down4x }; // 多 downsample 几次可以显著提高效率
		PNGBuffer mid{ width_down4x, height_down4x };
		PNGBuffer blur{ width_down4x, height_down4x };
		
		PNGBuffer gray{ width_down4x, height_down4x }; // 先转成灰度，再只计算r通道
		PNGBuffer Gxh{ width_down4x, height_down4x };
		PNGBuffer Gyh{ width_down4x, height_down4x };
		PNGBuffer Gx{ width_down4x, height_down4x };
		PNGBuffer Gy{ width_down4x, height_down4x };
		PNGBuffer G{ width_down4x, height_down4x }; // 其实同大小的不用了的Buffer是可以复用的，但是那样命名就太乱了
		PNGBuffer outdown2x{ width_down4x << 1, height_down4x << 1 };
		PNGBuffer out{ width_down4x << 2, height_down4x << 2 }; // 不能用in.width，因为奇数行列缩放会丢弃最后一行列像素

		// Multi Threads
		multi_threads_working<DownSample2x>(in, half);
		multi_threads_working<DownSample2x>(half, down4x);
		multi_threads_working<GaussionBlur1st>(down4x, mid); // 2-pass for sync
		multi_threads_working<GaussionBlur2nd>(mid, blur);
		
		multi_threads_working<GrayScale>(blur, gray);
		multi_threads_working<EdgeDetection1st>(gray, Gxh, ref(Gyh));
		multi_threads_working<EdgeDetection2ndX>(Gxh, Gx);
		multi_threads_working<EdgeDetection2ndY>(Gyh, Gy);
		multi_threads_working<EdgeDetection3rd>(Gx, G, cref(Gy));
		multi_threads_working<UpSample2x>(G, outdown2x);
		multi_threads_working<UpSample2x>(outdown2x, out);

		// Write File
		static const string adder = "_edge";
		out.WriteBufferToFile(path{ to.string() + rename(p.filename(), adder) });

		cout << "Finished.\n";
	}
}

void PNGManager::RunBright(unsigned char thresold)
{
	for (const path& p : pngFilePaths) {
		cout << "//////////////////////////////////////////////////////////////\n"
			<< "Processing " << p.string() << "\n";

		// Read File
		PNGBuffer in{ p };
		if (!in.state) continue;

		// 1.DownSample
		// 3.GrayScale
		// 4.BrightnessExtraction
		// 5.UpSample

		// Create Buffer
		unsigned width_half = in.width / 2;
		unsigned height_half = in.height / 2;
		PNGBuffer half{ width_half, height_half };
		PNGBuffer gray{ width_half, height_half };
		PNGBuffer outdown2x{ width_half, height_half };
		PNGBuffer out{ width_half << 1, height_half << 1 };

		// Multi Threads
		multi_threads_working<DownSample2x>(in, half);
		multi_threads_working<GrayScale>(half, gray);
		multi_threads_working<BrightnessExtraction>(gray, outdown2x, thresold);
		multi_threads_working<UpSample2x>(outdown2x, out);

		// Write File
		static const string adder = "_bright";
		out.WriteBufferToFile(path{ to.string() + rename(p.filename(), adder) });

		cout << "Finished.\n";
	}
}

void PNGManager::RunAll(unsigned char thresold)
{
	for (const path& p : pngFilePaths) {
		cout << "//////////////////////////////////////////////////////////////\n"
			<< "Processing " << p.string() << "\n";

		// Read File
		PNGBuffer in{ p };
		if (!in.state) continue;

		// All = GaussionBlur + EdgeDetection + BrightnessExtraction
		
		// Create Buffer
		unsigned width_half = in.width / 2;
		unsigned height_half = in.height / 2;
		PNGBuffer half{ width_half, height_half };
		PNGBuffer blurH{ width_half, height_half };
		PNGBuffer blur{ width_half, height_half };

		PNGBuffer gray{ width_half, height_half };
		PNGBuffer Gxh{ width_half, height_half };
		PNGBuffer Gyh{ width_half, height_half };
		PNGBuffer Gx{ width_half, height_half };
		PNGBuffer Gy{ width_half, height_half };
		PNGBuffer G{ width_half, height_half };
		PNGBuffer edge{ width_half << 1, height_half << 1 };

		PNGBuffer brighthalf{ width_half, height_half };
		PNGBuffer bright{ width_half << 1, height_half << 1 };

		// Multi Threads
		multi_threads_working<DownSample2x>(in, half);
		multi_threads_working<GaussionBlur1st>(half, blurH);
		multi_threads_working<GaussionBlur2nd>(blurH, blur);

		auto lamb = [to = to, p = p, rename = rename](PNGBuffer& out, string adder) {
			out.WriteBufferToFile(path{ to.string() + rename(p.filename(), adder) });
		};
		thread tblur(lamb, ref(blur), "_blur"s); // 写入文件单独开线程
		
		multi_threads_working<GrayScale>(blur, gray);
		multi_threads_working<EdgeDetection1st>(gray, Gxh, ref(Gyh));
		multi_threads_working<EdgeDetection2ndX>(Gxh, Gx);
		multi_threads_working<EdgeDetection2ndY>(Gyh, Gy);
		multi_threads_working<EdgeDetection3rd>(Gx, G, cref(Gy));
		multi_threads_working<UpSample2x>(G, edge);
		thread tedge(lamb, ref(edge), "_edge"s);

		multi_threads_working<BrightnessExtraction>(gray, brighthalf, thresold);
		multi_threads_working<UpSample2x>(brighthalf, bright);
		thread tbright(lamb, ref(bright), "_bright"s);

		// Wait writing file
		tblur.join();
		tedge.join();
		tbright.join();
		
		cout << "Finished.\n";
	}
}

inline bool PNGManager::is_png(const path& p)
{
	if (!exists(p) || !is_regular_file(p)) {
		return false;
	}
	static const unordered_set<string> acceptedExt = { ".png", ".PNG", ".Png" };
	if (acceptedExt.count(p.extension().string()) == 0) {
		return false;
	}
	return true;
}

inline string PNGManager::rename(const path& filename, const string& adder)
{
	string s = filename.string();
	int n = s.rfind(filename.extension().string());
	return s.substr(0, n) + adder + s.substr(n);
}

template<class _Op, class... _Valty>
void PNGManager::multi_threads_working(const PNGBuffer& in, PNGBuffer& out, _Valty&&... _Val)
{
	vector<thread> threads;
	const unsigned _size = out.width * out.height;
	const unsigned _block = _size / nCPUs + (_size % nCPUs != 0); // 1 block per thread
	for (unsigned i = 0; i < nCPUs; ++i) {
		const unsigned _begin = i * _block << 2;
		const unsigned _end = min((i + 1) * _block << 2, out.width * out.height << 2); // the last block end need clamp
		threads.emplace_back(thread(_Op{}, cref(in), ref(out), _begin, _end, forward<_Valty>(_Val)...));
	}
	cout << "Running " << typeid(_Op).name() << " on " << threads.size() << " threads...\n";
	for (auto& th : threads) {
		th.join();
	}
}
