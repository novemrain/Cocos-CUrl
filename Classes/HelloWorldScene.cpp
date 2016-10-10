#include "HelloWorldScene.h"
#include "../../../curl/include/win32/curl/curl.h"
#include "../../../curl/include/win32/curl/easy.h"
USING_NS_CC;

#define IMG_NAME "curl_test3.jpg"
#define	TAG_LABEL 99

HelloWorld::HelloWorld()
	: m_pWorkThread(nullptr)
{}
HelloWorld::~HelloWorld()
{
	CC_SAFE_DELETE(m_pWorkThread);
}

static size_t downLoadMethod(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	FILE *fp = (FILE*)userdata;
	size_t written = fwrite(ptr, size, nmemb, fp);
	return written;
}
int progressMethod(void *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)
{
	static int percent = 0;
	int tmp = (int)(nowDownloaded / totalToDownload * 100);

	if (percent != tmp)
	{
		percent = tmp;

		HelloWorld* layer = (HelloWorld*)ptr;
		cocos2d::Label* _label = (cocos2d::Label*)layer->getChildByTag(TAG_LABEL);

		Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]{
			std::string text = "percent is ";// +percent;
			std::stringstream ss;
			ss << percent;
			text = text + ss.str();
			_label->setString(text.c_str());
			CCLOG("downloading... %d%%", percent);
		});
	}
	return 0;
}
Scene* HelloWorld::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}
// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Layer::init() )
    {
        return false;
    }
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 2. add a "click" icon to download
    auto clickItem = MenuItemImage::create(
                                           "CloseNormal.png",
                                           "CloseSelected.png",
                                           CC_CALLBACK_1(HelloWorld::clickCallback, this));
	clickItem->setPosition(Vec2(origin.x + visibleSize.width - clickItem->getContentSize().width * 2 ,
		origin.y + clickItem->getContentSize().height * 2));
	clickItem->setScale(2);

    // create menu, it's an autorelease object
	auto menu = Menu::create(clickItem, NULL);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 1);

    /////////////////////////////
    // 3. add your codes below...
    auto label = Label::createWithTTF("C-url TEST", "fonts/Marker Felt.ttf", 24);
    
    // position the label on the center of the screen
    label->setPosition(Vec2(origin.x + visibleSize.width/2,
                            origin.y + visibleSize.height - label->getContentSize().height));

    // add the label as a child to this layer
    this->addChild(label, 1);
	label->setTag(TAG_LABEL);
	label->runAction(CCRepeatForever::create(CCSequence::createWithTwoActions(MoveBy::create(0.5, Vec3(100, 0, 0)), MoveBy::create(0.5, Vec3(-100, 0, 0)))));
	
    return true;
}
void HelloWorld::clickCallback(Ref* pSender)
{
	CCLOG("HelloWorld::clickCallback begin ");
	const std::string path = FileUtils::getInstance()->getWritablePath() + IMG_NAME;
	if (FileUtils::getInstance()->isFileExist(path))
	{
		CCLOG("HelloWorld::clickCallback %s", path.c_str());
		cocos2d::Label* _label = (cocos2d::Label*)getChildByTag(TAG_LABEL);
		_label->setString("Already Exist!!");

		Sprite* img = Sprite::create(path.c_str());
		Size visibleSize = Director::getInstance()->getVisibleSize();
		img->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
		this->addChild(img);
	}
	else
	{
		// make a new thread to do download
		m_pWorkThread = new std::thread(&HelloWorld::download, this);
		m_pWorkThread->detach();
	}
}
void HelloWorld::download()
{
	// Create a file to save package.
	std::string path = FileUtils::getInstance()->getWritablePath();
	const std::string outFileName = path + IMG_NAME;
	FILE *fp = fopen(outFileName.c_str(), "wb");
	if (!fp)
	{
		CCLOG("can not create file");
		return;
	}


	// Download pacakge
	// 注意curl库函数调用顺序!和参数设置
	void* _curl = curl_easy_init();
	std::string url = "http://img.25pp.com/uploadfile/soft/images/2013/1020/20131020020135604.jpg";
	//std::string url = "http://sw.bos.baidu.com/sw-search-sp/software/f11c9bcd24cfd/BaiduYunGuanjia_5.4.10.1.exe";

	curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
	// 保存写入相关参数
	curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, downLoadMethod);		//回调函数
	curl_easy_setopt(_curl, CURLOPT_WRITEDATA, fp);						//回调参数
	// 下载进度进度相关参数
	curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, false);					//是否"不显示进度"
	curl_easy_setopt(_curl, CURLOPT_PROGRESSFUNCTION, progressMethod);	//回调函数
	curl_easy_setopt(_curl, CURLOPT_PROGRESSDATA, this);				//回调参数
	
	curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1);

	// 执行请求,后释放
	CURLcode res = curl_easy_perform(_curl);
	curl_easy_cleanup(_curl);

	if (res != 0)
	{		
		CCLOG("error when download package %d", res);
	}
	else
	{
		Director::getInstance()->getScheduler()->performFunctionInCocosThread([outFileName, this]{
			// outFileName 必须传拷贝值不能传引用
			// 函数结束时thread关闭 释放资源,源引用不复存在!
			Sprite* img = Sprite::create(outFileName.c_str());

			Size visibleSize = Director::getInstance()->getVisibleSize();
			img->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
			this->addChild(img);
		});
		CCLOG("succeed downloading package %s", outFileName.c_str());
	}

	// 最后关闭文件流
	fclose(fp);
}



//void HelloWorld::menuCloseCallback2(Ref* pSender)
//{
//	auto sharedFileUtils = FileUtils::getInstance();
//
//	std::string ret;
//
//	sharedFileUtils->purgeCachedEntries();
//	std::string writablePath = sharedFileUtils->getWritablePath();
//	std::string fileName = writablePath + "external.txt";
//	char szBuf[100] = "Hello Cocos2d-x!";
//	FILE* fp = fopen(fileName.c_str(), "wb");
//	if (fp)
//	{
//		log("HelloWorld::menuCloseCallback2 fp %s", writablePath.c_str());
//		size_t ret = fwrite(szBuf, 1, strlen(szBuf), fp);
//		CCASSERT(ret != 0, "fwrite function returned zero value");
//		fclose(fp);
//		if (ret != 0)
//			log("Writing file to writable path succeed.");
//	}
//	else
//	{
//		log("HelloWorld::menuCloseCallback2 Writing file to writable path fail2.");
//	}
//
//
//}
//void HelloWorld::menuCloseCallback(Ref* pSender)
//{
//	FILE *fp;
//	auto sharedFileUtils = FileUtils::getInstance();
//	std::string writablePath = sharedFileUtils->getWritablePath();
//	std::string fileName = writablePath + "external.txt";
//
//	char str[11];
//	if ((fp = fopen(fileName.c_str(), "rt")) == NULL)
//	{
//		log("Cannot open file strike any key exit!");
//	}
//	fgets(str, 11, fp);
//	log("HelloWorld::menuCloseCallback %s", str);
//	fclose(fp);
//
//	m_pWorkThread = new std::thread(&HelloWorld::download, this);
//	m_pWorkThread->detach();
//
//	//m_pWorkThread = new std::thread(&HelloWorld::download, this);
//	//m_pWorkThread->detach();
//}