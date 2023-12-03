//opencv
#include <opencv2/opencv.hpp>
//sdl
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
//std
#include <string>

#define crop(mn,x,mx) (x < mn ? mn : (x > mx ? mx : x))

//sdl window
SDL_Window* window;
SDL_Renderer* renderer;

//image and window size
const int IMAGE_HEIGHT = 480;
const int IMAGE_WIDTH = 640;
const int WIDTH = IMAGE_WIDTH*2;
const int HEIGHT = IMAGE_HEIGHT + 260;
const int FPS = 60;

//thresholds
int threshold = 10;//used for both the difference in color in edges and frames
int colThreshold = 10;//used to tone down color

//init sdl, set sdl window and renderer
void init()
{
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Edge Detection", 0, 0, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	TTF_Init();
}
//quit sdl
void quit()
{
	TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}


void drawText(int x, int y, TTF_Font *font, const char* text)
{
	SDL_Surface* surfaceMessage = TTF_RenderText_Blended(font, text, {0, 0, 0, 255});
	SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = surfaceMessage->w;
	src.h = surfaceMessage->h; 
	
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	dst.w = src.w;
	dst.h = src.h;

	SDL_RenderCopy(renderer, message, &src, &dst);
	SDL_FreeSurface(surfaceMessage);
	SDL_DestroyTexture(message);
}

int main()
{
	//init sdl
	init();
	
	//open camera
	cv::VideoCapture cap(0, cv::CAP_V4L);
	if(!cap.isOpened())
	{
		std::cerr << "Error: Couldn't open the camera." << std::endl;
		return -1;
	}
	//set camera stream size and fps
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, IMAGE_HEIGHT);
	cap.set(cv::CAP_PROP_FRAME_WIDTH, IMAGE_WIDTH);
	cap.set(cv::CAP_PROP_FPS, FPS);
	//print stream size and fps
	std::cout << "CAP_PROP_FPS : " << cap.get(cv::CAP_PROP_FPS) << std::endl;
	std::cout << "CAP_PROP_FRAME_HEIGHT : " << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
	std::cout << "CAP_PROP_FRAME_WIDTH : " << cap.get(cv::CAP_PROP_FRAME_WIDTH) << std::endl;

	//create sdl texture and its parent rect
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, IMAGE_WIDTH, IMAGE_HEIGHT);
	SDL_Rect destination = {0, 0, IMAGE_WIDTH, IMAGE_HEIGHT};
	
	//alocate memory for sdl texture to be used as edge image
	SDL_Rect destination_edge = {IMAGE_WIDTH, 0, IMAGE_WIDTH, IMAGE_HEIGHT};
	SDL_Texture* edges = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, IMAGE_WIDTH, IMAGE_HEIGHT);
	
	//open font and rect for text
	TTF_Font* font = TTF_OpenFont("FreeMono.ttf", 40);
	SDL_Rect fps_rect = {20, IMAGE_HEIGHT+10, 0, 0};
	SDL_Rect thr_rect = {20, IMAGE_HEIGHT+60, 0, 0};
	SDL_Rect rect = {0, IMAGE_HEIGHT, WIDTH, HEIGHT-IMAGE_HEIGHT};
	
	//calculate fps
	Uint32 frameStart;
	int frameTime;
	int fps = 0;
	
	
	cv::Mat frame;
	cv::Mat frame2;
	cap.read(frame);
	//switches
	bool run = true;
	bool flip = true;//Movement or edge
	bool flipc = true;//color or black and white
	//stream img
	while(run)
	{
		//set start of frame
		frameStart = SDL_GetTicks();
		
		//input
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			switch(e.type)
			{
				case SDL_QUIT:
					run = false;
					break;
				case SDL_KEYDOWN:
					switch(e.key.keysym.sym)
					{
						case SDLK_a:
							threshold += 1;
							break;
						case SDLK_s:
							if (threshold > 0)
							{
								threshold -= 1;
							}
							break;
						case SDLK_e:
							colThreshold += 1;
							break;
						case SDLK_r:
							if (colThreshold > 1)
							{
								colThreshold -= 1;
							}
							break;
						case SDLK_f:
							flip = !flip;
							break;
						case SDLK_g:
							flipc = !flipc;
							break;
					}
					break;
			}
		}

		//get frame
		frame2 = frame.clone();
		cap.read(frame);

		//flip image
		cv::flip(frame, frame, 1);
		
		//update the streaming texture with the new pixel data and detect edges
		void* pixels;
		int pitch;
		long t = threshold*threshold;
		SDL_LockTexture(edges, NULL, &pixels, &pitch);
		Uint32* pixelData = (Uint32*)pixels;
		//get cur Pixel Data
		cv::Vec3b* curPixel_Base = &frame.at<cv::Vec3b>(1, 1);
		//init "neighbour" pixels -> Bases used later to iterate through everything without having to recreate the pixel
		cv::Vec3b* PixelL_Base;
		cv::Vec3b* PixelR_Base;
		cv::Vec3b* PixelT_Base;
		cv::Vec3b* PixelB_Base;

		if (flip) {
			//set "neighbour" pixels as neighbours
			PixelL_Base = &frame.at<cv::Vec3b>(1, 1-1);
			PixelR_Base = &frame.at<cv::Vec3b>(1, 1+1);
			PixelT_Base = &frame.at<cv::Vec3b>(1-1, 1);
			PixelB_Base = &frame.at<cv::Vec3b>(1+1, 1);
		} else {
			//set "neighbour" pixels as curent pixels
			PixelL_Base = &frame2.at<cv::Vec3b>(1, 1);
			PixelR_Base = &frame2.at<cv::Vec3b>(1, 1);
			PixelT_Base = &frame2.at<cv::Vec3b>(1, 1);
			PixelB_Base = &frame2.at<cv::Vec3b>(1, 1);
		}

		for(int y = 1; y < IMAGE_HEIGHT-1; y++)
		{
			int index = y *(pitch/sizeof(Uint32))+1;
			//set Curent index of pixels -> Use Base pixels as a start place to iterate through image
			cv::Vec3b* curPixel = curPixel_Base;
			cv::Vec3b* PixelL = PixelL_Base;
			cv::Vec3b* PixelR = PixelR_Base;
			cv::Vec3b* PixelT = PixelT_Base;
			cv::Vec3b* PixelB = PixelB_Base;

			for(int x = 1; x < IMAGE_WIDTH-1; x++)
			{
				//find diference in color between all "neighbouring" and curent pixel
				cv::Vec3s diffL = *curPixel-*PixelL;//curPixel and PixelL are pointers, you have to put a star in front in order to turn them back into objects
				cv::Vec3s diffR = *curPixel-*PixelR;//curPixel and PixelR are pointers, you have to put a star in front in order to turn them back into objects
				cv::Vec3s diffT = *curPixel-*PixelT;//curPixel and PixelT are pointers, you have to put a star in front in order to turn them back into objects
				cv::Vec3s diffB = *curPixel-*PixelB;//curPixel and PixelB are pointers, you have to put a star in front in order to turn them back into objects
				if(flipc)
				{ 
					//find dot product (square) and compare it to the threashold
					if ((diffL.dot(diffL) > t)
					 || (diffR.dot(diffR) > t)
					 || (diffT.dot(diffT) > t)
					 || (diffB.dot(diffB) > t))
					{
						pixelData[index] = 0x000000FF;//set white
					} else
					{
						pixelData[index] = 0xFFFFFFFF;//set black
					}
				}else
				{
					//add all differences (add all rgb in all differences) together, calculate an rgb then transfer it to hexidecimal
					cv::Vec3s diffSum = diffL + diffR + diffT + diffB;
					uint32_t r = 127 + diffSum[0]*threshold/colThreshold;
					uint32_t g = 127 + diffSum[1]*threshold/colThreshold;
					uint32_t b = 127 + diffSum[2]*threshold/colThreshold;
					pixelData[index] = (crop(0,r,255)<<24) + (crop(0,g,255)<<16) + (crop(0,b,255)<<8) + 255;
				}
				//add to all indexes
				index ++;
				curPixel ++;
				PixelL ++;
				PixelR ++;
				PixelT ++;
				PixelB ++;
			}
			//update bases to match new line in image
			curPixel_Base += IMAGE_WIDTH;
			PixelL_Base += IMAGE_WIDTH;
			PixelR_Base += IMAGE_WIDTH;
			PixelT_Base += IMAGE_WIDTH;
			PixelB_Base += IMAGE_WIDTH;
		}
		SDL_UnlockTexture(edges);

		//convert to sdl
		SDL_UpdateTexture(texture, NULL, frame.data, frame.step);

		//show text
		SDL_RenderFillRect(renderer, &rect);
		std::string str = "FPS: " + std::to_string(fps);
		drawText(fps_rect.x, fps_rect.y, font, str.c_str());
		str = "Threshold: " + std::to_string(threshold);
		drawText(thr_rect.x, thr_rect.y, font, str.c_str());
		str = "Color Threshold: " + std::to_string(colThreshold);
		drawText(thr_rect.x, thr_rect.y+50, font, str.c_str());
		if(flip)
			drawText(20, thr_rect.y+100, font, "Edge Detection");
		else
			drawText(20, thr_rect.y+100, font, "Movement Detection");
		if(flipc)
			drawText(20, thr_rect.y+150, font, "No Color");
		else
			drawText(20, thr_rect.y+150, font, "Color");
		
		//show img
		SDL_RenderCopy(renderer, texture, NULL, &destination);
		
		//show edge img
		SDL_RenderCopy(renderer, edges, NULL, &destination_edge);
		SDL_RenderPresent(renderer);
		
		//end of frame time
		frameTime = SDL_GetTicks() - frameStart;
		fps = int(1000.0f/frameTime);
	}

	//release opencv camera
	cap.release();
	//remove ttf
	TTF_CloseFont(font);
	//remove images
	SDL_DestroyTexture(edges);
	SDL_DestroyTexture(texture);
	//quit sdl
	quit();
	return 0;
}
