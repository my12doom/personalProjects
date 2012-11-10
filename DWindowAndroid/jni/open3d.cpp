#include <cerrno>
#include <cstddef>
#include <string.h>
#include <jni.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <android/log.h>
#include <time.h>
#include <pthread.h>

#define  LOG_TAG    "com_Vstar_Open3d"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
//#define LOGI(...) ;
//#define LOGE(...) ;


#include "stdio.h"

#define CLASS com_Vstar_Open3d
#define CLSSS2 com_player_MobileMng
#define CLASST com_V3dvstar_GetClassList_GetClasslist
#define NAME2(CLZ, FUN) Java_##CLZ##_##FUN
#define NAME1(CLZ, FUN) NAME2(CLZ, FUN)
#define NAME(FUN) NAME1(CLASS,FUN)
#define NAMEj(FUN) NAME1(CLASS2,FUN)
#define NAMET(FUN) NAME1(CLASST,FUN)

void HttpDown(int64_t Range,char *hostname,int port,char *path);
const char *g_imei = NULL;		// warning: memory leak
const char *g_imsi = NULL;
const char *g_model = NULL;
const char *g_MAC = NULL;
int g_version = 0;
int g_deny = 0;

enum Brand3D
{
	Brand_No3D = 0,
	Brand_HTC = 1,
	Brand_LG = 2,
	Brand_Sharp = 3,
	Brand_3dvstar = 4,
	Brand_ZOP = 5,		// 6575 @ android 2.3.4
	Brand_MTK = 6,		// 6577
};

extern "C" 
{
	int m_Brand3D = Brand_No3D;
	jclass  m_SurfaceController=NULL;
	jobject m_SurfaceControllerobj=NULL;
	jobject m_displayservice=NULL;
	jobject m_obj=NULL;
	JNIEnv * m_env=NULL;
	char t2[200];
	char t3[200];
	char t4[200];
	char t5[200];
	bool InitHtcOpen3DMode(jobject loader);
	bool msg_Dbg(const char *b,int len)
	{
		FILE *fp;
		if (!(fp = fopen("/sdcard/3dv/open3d.txt", "a+b")))     
		 return false;
		 fwrite(b, sizeof(unsigned char),len, fp); 
		 fclose(fp);     
		return true;
	}


	char* decode(char *ostr)
	{
		
		const char* passcode="1102ratsvd3";
		int l=strlen(ostr);
		int ol=strlen(passcode);
		int i=0;
		int j=0;
		int o=0;
		j=(l/2)%ol-1;
		for(i=0;i<l;i+=2)
		{
			o=('z'-ostr[l-i-1])*26+('z'-ostr[l-i-2]);
			t2[i/2]=o ^ passcode[j]; 
			j--;
			if(j<0)
				j=ol-1;
		}
		t2[i/2]='\0';
		//t[i]='\0';
		return t2;
	}
	
	char* decode2(char *ostr)
	{
		
		const char* passcode="1102ratsvd3";
		int l=strlen(ostr);
		int ol=strlen(passcode);
		int i=0;
		int j=0;
		int o=0;
		j=(l/2)%ol-1;
		for(i=0;i<l;i+=2)
		{
			o=('z'-ostr[l-i-1])*26+('z'-ostr[l-i-2]);
			t3[i/2]=o ^ passcode[j]; 
			j--;
			if(j<0)
				j=ol-1;
		}
		t3[i/2]='\0';
		//t[i]='\0';
		return t3;
	}

	char* decode3(char *ostr)
	{
		
		const char* passcode="1102ratsvd3";
		int l=strlen(ostr);
		int ol=strlen(passcode);
		int i=0;
		int j=0;
		int o=0;
		j=(l/2)%ol-1;
		for(i=0;i<l;i+=2)
		{
			o=('z'-ostr[l-i-1])*26+('z'-ostr[l-i-2]);
			t4[i/2]=o ^ passcode[j]; 
			j--;
			if(j<0)
				j=ol-1;
		}
		t4[i/2]='\0';
		//t[i]='\0';
		return t4;
	}

	char* decode4(char *ostr)
	{
		
		const char* passcode="1102ratsvd3";
		int l=strlen(ostr);
		int ol=strlen(passcode);
		int i=0;
		int j=0;
		int o=0;
		j=(l/2)%ol-1;
		for(i=0;i<l;i+=2)
		{
			o=('z'-ostr[l-i-1])*26+('z'-ostr[l-i-2]);
			t5[i/2]=o ^ passcode[j]; 
			j--;
			if(j<0)
				j=ol-1;
		}
		t5[i/2]='\0';
		//t[i]='\0';
		return t5;
	}

	jclass GetClassLoader(char * classname, jobject loader)
	{
		jobject rObj=NULL;
		jthrowable excp = 0; 
		jclass pClass=m_env->GetObjectClass(loader);
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			delete pClass;
			pClass=NULL;
		}
		if(pClass==NULL)
			return NULL;
		
		jmethodID mid=NULL;
		mid = m_env->GetMethodID(pClass,decode3("lxlxwwjwcyuzezxyzy"),decode4("pzlxkxuwvyrymwfzbzuziwvyxwhxuwbzgykwfxizpznwkxixewwyezkzezuyowuzixxwmwfvzy")); // loadClass (Ljava/lang/String;)Ljava/lang/Class;
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return NULL;
		 if(loader==NULL)
			 return NULL;

		rObj=m_env->CallObjectMethod(loader,mid,m_env->NewStringUTF(classname));
		excp = m_env->ExceptionOccurred(); 
		if (excp) 
		{
			m_env->ExceptionClear();
			rObj=NULL;
		}        
		
		//m_env->DeleteLocalRef(DexClassLoader);
		//DexClassLoader=NULL;

		if(rObj==NULL)
			return NULL;
		return (jclass)rObj;


	}

	jclass LoadPackegedClass(char * Package,char * classname, jobject loader)
	{
		jclass pClass=NULL;
		jobject DexClassLoader =NULL;
		jobject rObj=NULL;
		jthrowable excp = 0;    
	

		pClass=m_env->FindClass(decode3("kxtwtwuwwygyszzzczrzrvextwnvwyuyvzzzzzkzczxynwpwhxjwgzuz"));//"dalvik/system/DexClassLoader");
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			delete pClass;
			pClass=NULL;
		}
		if(pClass==NULL)
			return NULL;
		
		jmethodID mid=NULL;
		mid = m_env->GetMethodID(pClass,decode3("kzixowlwyykw"),decode4("awbzoznxczuzezxytxcznxxwkwovwyezkzezuyowuzixxwmwfvqzezkzwyyzxzwxxyrwiwwwjwkwzzxzhzxylyrzrwiwownxtzbymwfzbzuziwvyxwhxuwbzgyywfzbzmzmxixfwuyswxyzzbzlwczhzvwmwevbz"));
		// "<init>","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");  
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return NULL;
		DexClassLoader = m_env->NewObject(pClass,mid,m_env->NewStringUTF(Package),m_env->NewStringUTF("/sdcard"/*decode3("mxlwjxwy")*/),NULL,loader);  ///tmp
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			DexClassLoader=NULL;
		}      
		if(DexClassLoader==NULL)
			return NULL;
		mid=NULL;
		mid=m_env->GetMethodID(pClass,decode3("lxlxwwjwcyuzezxyzy"),decode4("pzlxkxuwvyrymwfzbzuziwvyxwhxuwbzgykwfxizpznwkxixewwyezkzezuyowuzixxwmwfvzy")); // loadClass (Ljava/lang/String;)Ljava/lang/Class;
		excp = m_env->ExceptionOccurred(); 
		if (excp) 
		{
			m_env->ExceptionClear();
			DexClassLoader=NULL;
		}        
		if(mid==NULL)
			return NULL;
		
		rObj=m_env->CallObjectMethod(DexClassLoader,mid,m_env->NewStringUTF(classname));
		excp = m_env->ExceptionOccurred(); 
		if (excp) 
		{
			m_env->ExceptionClear();
			rObj=NULL;
		}        
		
		m_env->DeleteLocalRef(DexClassLoader);
		DexClassLoader=NULL;

		if(rObj==NULL)
			return NULL;
		return (jclass)rObj;

	}

	bool Open3DHTC(jboolean show,jobject mSurface)
	{
		return true;
		
		jthrowable excp = 0;    
		jmethodID mid=NULL;
		if (mSurface==NULL)
		{
			return false;
		}
		if(m_SurfaceController==NULL)
			return false;

		mid = m_env->GetStaticMethodID(m_SurfaceController,decode2("ixxwkwnxwymydynxezmzkxjwvwkxkwczgzizszoyjzrwlx"),decode("wvbzivqzczxzezezvzizhwvyhxswmwvzzwjzzyazdzqwiwxwfvzy")); 
		//setStereoscopic3DFormat (Landroid/view/Surface;I)Z
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
		if(show==0)
			m_env->CallStaticBooleanMethod(m_SurfaceController,mid,mSurface,0);
		else
			m_env->CallStaticBooleanMethod(m_SurfaceController,mid,mSurface,1);
		
		return true;

	}
    

	bool Open3DSharp(jboolean show,jobject faceview)
	{
		LOGE("Open3DSharp");
		jthrowable excp = 0;    
		jmethodID mid=NULL;

		if(m_SurfaceController==NULL)
			return false;
		if(m_SurfaceControllerobj==NULL)
		{
			
			mid = m_env->GetMethodID(m_SurfaceController, decode2("kzixowlwyykw"),decode("awbzozixczrzrydzezuzswkxjxewwyuzvzwyuzowzznwjwkxtwlwgzgylw")); //(Landroid/view/SurfaceView;)V
			
			excp = m_env->ExceptionOccurred();     
			if (excp) 
			{
				m_env->ExceptionClear();
				mid=NULL;
			}   
			if(mid==NULL)
				return false;
			m_SurfaceControllerobj = m_env->NewObject(m_SurfaceController,mid,faceview);
			excp = m_env->ExceptionOccurred();    
			if (excp) 
			{
				m_env->ExceptionClear();
				m_SurfaceControllerobj=NULL;
			}   
					
		}
		
		if(m_SurfaceControllerobj==NULL)
			return false;
		
		mid=NULL;
		mid = m_env->GetMethodID(m_SurfaceController, decode2("hxtwowdwwyvztzdzxzwxgxtwlx"),decode("awbzxvzy")); //setStereoView (Z)V
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
			
		}   
		if(mid==NULL)
			return false;
	
		m_env->CallVoidMethod(m_SurfaceControllerobj,mid,show);
		
		return true;

	}

	bool Open3DSharp2(jboolean show,jobject faceview)
	{
		LOGE("Open3DSharp2");

		jthrowable excp = 0;    
		jmethodID mid=NULL;
	
		if(m_SurfaceController==NULL)
			return false;
		if(m_SurfaceControllerobj==NULL)
		{
			jclass castClass=m_env->FindClass(decode("lxlxwwjwcyywgzwyczrzwyxwgxwwpw"));//"java.lang.Class");
			m_env->ExceptionClear();  
			if(castClass==NULL)
				return false;
			mid=NULL;
			mid=m_env->GetMethodID(castClass,decode2("ixlxwwww"),decode("pzixuwqwbzwzsxlwizpzvwkwvywwjxgzozvxnwaxjzxwtwmwvwevkwtzzyhzzycxvwgxxwnwdvnw"));//"cast","(Ljava/lang/Object;)Ljava/lang/Object;"
			m_env->ExceptionClear();    
			if(mid==NULL)
				return false;
			m_SurfaceControllerobj=m_env->CallObjectMethod(m_SurfaceController,mid,faceview);
			excp = m_env->ExceptionOccurred(); 
			if (excp) 
			{
				m_env->ExceptionClear();
				m_SurfaceControllerobj=NULL;
			}        
	
		}
		
		if(m_SurfaceControllerobj==NULL)
			return false;
		mid=NULL;
		mid = m_env->GetMethodID(m_SurfaceController, decode2("hxtwowdwwyvztzdzxzwxgxtwlx"),decode("awbzxvzy")); //setStereoView (Z)V
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
			
		}   
		if(mid==NULL)
			return false;
	
		m_env->CallVoidMethod(m_SurfaceControllerobj,mid,show);
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			return false;
			
		}   

		return true;

	}
	char get16(int i)
	{
		return ('z'-i);
	}
	char t[200];
	
	char* ecode(char *ostr)
	{
		
		const char* passcode="1102ratsvd3";
		int l=strlen(ostr);
		int ol=strlen(passcode);
		int i=0;
		int j=0;
		int o=0;
		for(i=0;i<l;i++)
		{
			o=ostr[l-i-1] ^ passcode[j];
			t[i*2]=get16(o % 26);
			t[i*2+1]=get16(o / 26);
			//t[i]=~o;
			j++;
			if(j==ol)
				j=0;
		}
		t[i*2]='\0';
		//t[i]='\0';
		return t;
	}

	

	bool Open3DLG(jboolean show,jobject holder)
	{
		jthrowable excp = 0;    
		jmethodID mid=NULL;
	
		if(m_SurfaceController==NULL)
			return false;
		if(m_SurfaceControllerobj==NULL)
		{
			jclass castClass=m_env->FindClass(decode("lxlxwwjwcyywgzwyczrzwyxwgxwwpw"));//"java.lang.Class");
			m_env->ExceptionClear();  
			if(castClass==NULL)
				return false;
			mid=NULL;
			mid=m_env->GetMethodID(castClass,decode2("ixlxwwww"),decode("pzixuwqwbzwzsxlwizpzvwkwvywwjxgzozvxnwaxjzxwtwmwvwevkwtzzyhzzycxvwgxxwnwdvnw"));//"cast","(Ljava/lang/Object;)Ljava/lang/Object;"
			m_env->ExceptionClear();    
			if(mid==NULL)
				return false;
			m_SurfaceControllerobj=m_env->CallObjectMethod(m_SurfaceController,mid,holder);
			excp = m_env->ExceptionOccurred(); 
			if (excp) 
			{
				m_env->ExceptionClear();
				m_SurfaceControllerobj=NULL;
			}        
	
		}
		
		if(m_SurfaceControllerobj==NULL)
			return false;
		mid=NULL;
		mid = m_env->GetMethodID(m_SurfaceController, decode2("twmxexbwxxvwmyszgzcz"),decode("awbzivzy")); //setS3DType  (I)V
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
		if(show==0)
			m_env->CallVoidMethod(m_SurfaceControllerobj,mid,0);//0
		else
			m_env->CallVoidMethod(m_SurfaceControllerobj,mid,129); //129 //1
		
		return true;

	}

	//zp
	bool IsZP(jclass SurfaceController)
	{
		LOGE("IsZP(%08x)", SurfaceController);

		jthrowable excp = 0;    
		jmethodID mid=NULL;
	
		if(SurfaceController==NULL)
			return false;

		mid = m_env->GetMethodID(SurfaceController, "setLayerType","(II)V");
		excp = m_env->ExceptionOccurred();
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
		return true;
	}

	bool Open3DZp(jboolean show,jobject Surface, jobject surfaceView, bool issys)
	{
		jthrowable excp = 0;    
		jmethodID mid=NULL;

		if(surfaceView==NULL && !issys)
			return false;
		if(Surface==NULL && issys)
			return false;

		if (!issys)
			mid = m_env->GetMethodID(m_env->GetObjectClass(surfaceView), decode("ixixkwozzzvxwxixjzrwov")/*"set3DLayout"*/,
																		 decode3("awbzivzy")/*"(I)V"*/);
		else
			mid = m_env->GetMethodID(m_env->GetObjectClass(Surface), decode("twmxexbwzzvzmzhztxjzrwlx")/*"setLayerType"*/,
																	 decode3("awbzivgvnw")/*"(II)V"*/);
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;

		if(show==0)
		{
			if(!issys)
				m_env->CallVoidMethod(surfaceView,mid,1);//0
			else
				m_env->CallVoidMethod(Surface,mid,0xff00,1);//0
		}
		else
		{
			if(!issys)
				m_env->CallVoidMethod(surfaceView,mid,0x20); //129 //1
			else
				m_env->CallVoidMethod(Surface,mid,0xff00,0x20); //129 //1
			//m_env->CallVoidMethod(Surface,mid,0,0); //129 //1
		}

		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
		return true;
	}
	
	// 6577
	bool Open3D6577(jboolean show, jobject surfaceView)
	{
		jthrowable excp = 0;    
		jmethodID mid=NULL;

		if(surfaceView==NULL)
			return false;

		mid = m_env->GetMethodID(m_env->GetObjectClass(surfaceView), "set3DLayout",
																		 decode3("awbzivzy")/*"(I)V"*/);
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}
		if(mid==NULL)
			return false;

		if(show==0)
		{
				m_env->CallVoidMethod(surfaceView,mid,1);
		}
		else
		{
				m_env->CallVoidMethod(surfaceView,mid,0x20); 
		}

		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
		return true;
	}
		
	//6577
	bool Is6577()
	{
		LOGE("Is6577 1");
		jthrowable excp = 0;    
		jmethodID mid=NULL;
		
		jclass pClass=m_env->FindClass(decode3("hxtwowdwczxzezezvzizhwvyhxswmwvzzwjzzyazdzqwiwxw"));//"android/view/SurfaceView");
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			pClass=NULL;
		}
		if(pClass==NULL)
			return false;

		LOGE("Is6577 2");
		mid = m_env->GetMethodID(pClass, "set3DLayout","(I)V");
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		LOGE("Is6577 3");
		if(mid==NULL)
			return false;
		LOGE("Is3DV2 4");
		return true;
	}
	
	//3DV
	bool Is3DV(jclass SurfaceController)
	{
		jthrowable excp = 0;    
		jmethodID mid=NULL;
	
		if(SurfaceController==NULL)
			return false;

		mid = m_env->GetMethodID(SurfaceController, "SetV3DType","(III)V");
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
		return true;
	}
	
	//3DV
	bool Is3DV2()
	{
		LOGE("Is3DV2 1");
		jthrowable excp = 0;    
		jmethodID mid=NULL;
		
		jclass pClass=m_env->FindClass(decode3("twvwwwtwzzfzmylwyzyznwgxvytwmwwygzjzwycz"));//"android/view/Surface");
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			pClass=NULL;
		}
		if(pClass==NULL)
			return false;

		LOGE("Is3DV2 2");
		mid = m_env->GetMethodID(pClass, "SetV3DType","(III)V");
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		LOGE("Is3DV2 3");
		if(mid==NULL)
			return false;
		LOGE("Is3DV2 4");
		return true;
	}

	bool Open3D3DV(jboolean show,jobject Surface, jobject surfaceView, bool is3dv)
	{
		LOGE("Open3D 3DV(%d,%08x,%08x,%d)", show, Surface, surfaceView, is3dv);

		jthrowable excp = 0;    
		jmethodID mid=NULL;

		if(Surface==NULL)
			return false;

		mid = m_env->GetMethodID(m_env->GetObjectClass(Surface), decode("twmxexbwxxvwryszgzwx")/*"SetV3DType"*/,
																 decode3("awbzivgvsxex")/*"(III)V"*/);
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;

		if(show==0)
		{
			if(is3dv)
			{
				LOGE("1,0,0");
				m_env->CallVoidMethod(Surface,mid,1,0,0);
			}
			else
			{
				LOGE("0,0,0");
				m_env->CallVoidMethod(Surface,mid,0,0,0);
			}
		}
		else
		{
			LOGE("1,3,0");
			m_env->CallVoidMethod(Surface,mid,1,3,0);
		}

		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;

		LOGE("Open3D 3DV() returns true");
		return true;
	}

	bool Open3DLANDSCAPE(jboolean issys,int n,jobject mSurface, int n2)
	{
		LOGE("643");
		jthrowable excp = 0;    
		jmethodID mid=NULL;
	
		if(m_SurfaceController==NULL)
			return false;
		if(m_displayservice==NULL)
			return false;
		
			LOGE("652");
			
		jclass pClass=m_env->FindClass(decode3("rwiwowhxtzvzmypzczrzkxlxpwnvwyuzvzwyuzowszgxowvykwkwiz"));//"com/htc/view/DisplaySetting");
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			delete pClass;
			pClass=NULL;
		}
		if(pClass==NULL)
			return NULL;
		
		mid = m_env->GetStaticMethodID(pClass,"setStereoscopic3DFormat", "(Landroid/view/Surface;I)Z");
		excp = m_env->ExceptionOccurred();
		if (excp)
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
			
		LOGE("793");
		m_env->CallStaticBooleanMethod(pClass,mid,mSurface,n2);
		excp = m_env->ExceptionOccurred();     
		if (excp)
		{
			m_env->ExceptionClear();
			return false;
		}   
		
		return true;
		// 
		
		LOGE("6522");
		if(issys==1)
			mid = m_env->GetMethodID(m_SurfaceController,decode2("twswiwcvozzzbzwzuzmzkvjwtwlxqwtzbyzzdzuz"),"(ILjava/lang/String;)I");//"setStereoDisplayMode"
		else
			mid = m_env->GetMethodID(m_SurfaceController,decode2("twswiwcvxxvwzzdzuz"),"(ILjava/lang/String;)I");//set3DMode
			
		LOGE("658");

		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
		LOGE("668");
		m_env->CallIntMethod(m_displayservice,mid,n,m_env->NewStringUTF(decode2("kxxwjxmxvzuzgx")));
		excp = m_env->ExceptionOccurred();
		if (excp) 
		{
			m_env->ExceptionClear();
			return false;
		} 

		LOGE("677");

		return true;
	}
	bool InitHtcOpen3DMode(jobject loader)
	{
		//if(m_displayservice!=NULL)
		//	return true;
		jthrowable excp = 0;    
		jclass  ServiceManagerclass=NULL;
		jobject displayIserver=NULL;
		//jobject m_SurfaceControllerobj=NULL;

		ServiceManagerclass=GetClassLoader(/*decode2("kxxwnwxyizczszlwowkzmxjwhxswiwgzgzhzlwyyyzgxlxfxkxwykw"),*/decode("kxtwqwuwxyzzuxdzezmzixkxtwewxyyzlznwczuyozmxswiwww"), loader);//"android.os.ServiceManager");
		if(ServiceManagerclass==NULL)
			return false;
		jmethodID mid=NULL;
		mid = m_env->GetStaticMethodID(ServiceManagerclass,decode2("twvwowjxzzvzmyszgzwz"),decode("pzkxswrwxyrzxxtxowczlwvyswowkwzzuzzyhztxaxrzrwiwownxtzbymwfzbzuziwvyxwhxuwbzgylw"));//)"getService","(Ljava/lang/String;)Landroid/os/IBinder;"); //setStereoscopic3DFormat (Landroid/view/Surface;I)Z
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;
		displayIserver=m_env->CallStaticObjectMethod(ServiceManagerclass,mid,m_env->NewStringUTF(decode2("fxxwlwlxyzrzjz")));//display
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			displayIserver=NULL;
		}   
		if(displayIserver==NULL)
			return false;
		ServiceManagerclass=NULL;
		ServiceManagerclass=GetClassLoader(/*decode2("kxxwnwxyizczszlwowkzmxjwhxswiwgzgzhzlwyyyzgxlxfxkxwykw"),*/decode("uwjxjxgwrwvzczzyzzdzrwfwfxwwjwxzhzwywxoxdxnxjwuytwmwwygzjzwycz"), loader);//"android.os.IDisplayService$Stub"
		if(ServiceManagerclass==NULL)
			return false;
		mid=NULL;
		mid = m_env->GetStaticMethodID(ServiceManagerclass,"asInterface",decode("pztwuwmwvzgziztykzuziwmxlxowlvsxzwszxyowzznwjwkxtwlwgzgykwfxvzyzqwiwpwpvgvkwhzyylwhzmzlwkxswjwuwpxex")); //setStereoscopic3DFormat (Landroid/view/Surface;I)Z
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		if(mid==NULL)
			return false;

		LOGE("720");

		if(m_displayservice!=NULL)
		{
			LOGE("724");
			//m_env->DeleteLocalRef(m_displayservice);
			m_displayservice=NULL;
			LOGE("727");
		}
		m_displayservice=m_env->CallStaticObjectMethod(ServiceManagerclass,mid,displayIserver);
		LOGE("728");
		if (excp) 
		{
			m_env->ExceptionClear();
			m_displayservice=NULL;
		}   
		if(m_displayservice==NULL)
			return false;
		
		LOGE("737");
		m_SurfaceController=m_env->GetObjectClass(m_displayservice);
		if(m_SurfaceController==NULL)
			return false;
		mid = m_env->GetMethodID(m_SurfaceController,decode("twswiwcvxxvwzzdzuz"),"(ILjava/lang/String;)I");
		excp = m_env->ExceptionOccurred();     
		if (excp) 
		{
			m_env->ExceptionClear();
			mid=NULL;
		}   
		LOGE("748");
		if(mid==NULL)
			return false;
		if(displayIserver!=NULL)
		{
			LOGE("750");
			m_env->DeleteLocalRef(displayIserver);
			LOGE("752");
			displayIserver=NULL;
		}

		return true;
	}

	void outecode(char * aa)
	{
		msg_Dbg(aa,strlen(aa));
		msg_Dbg("\n\r",2);
		msg_Dbg("\n\r",2);
		msg_Dbg(ecode(aa),strlen(aa)*2);
		msg_Dbg("\n\r",2);
		msg_Dbg("\n\r",2);
		msg_Dbg(decode(ecode(aa)),strlen(aa));
		msg_Dbg("\n\r",2);
		msg_Dbg("\n\r",2);
		msg_Dbg("\n\r",2);
		msg_Dbg("\n\r",2);
	}
	JNIEXPORT jint NAMET(InItHtc)( JNIEnv * env, jobject obj,jobject Loader)
	{
		m_env=env;
		m_obj=obj;
		//outecode("android.view.SurfaceView");
	/*	if(InitHtcOpen3DMode())
		{
			/*outecode("set3DMode");
			outecode("(ILjava/lang/String;)I");
			outecode("asInterface");
			outecode("(Landroid/os/IBinder;)Landroid/os/IDisplayService;");
			outecode("android.os.IDisplayService$Stub");
			outecode("display");
			outecode("getService");
			outecode("(Ljava/lang/String;)Landroid/os/IBinder;");
			outecode("android.os.ServiceManager");
			outecode("3dvstar");
			outecode("set3DMode");
			outecode("(ILjava/lang/String;)I");
			outecode("setStereoDisplayMode");
			outecode("(ILjava/lang/String;)I");
			outecode("android.media.MediaPlayer");*/
			//Open3DLANDSCAPE(0);
		//	return 1;
		//}
		//else
			return -1;
	}

	
	void  *start_routine(void *arg)
	{

		sleep(10);

		srand(time(NULL));

		char str[256] = "";
		//strcat(str, g_imei?g_imei:"NULL");
		//strcat(str, "/");
		//strcat(str, g_imsi?g_imsi:"NULL");
		sprintf(str, decode3("lxfzmzjwczuzyyvyxwzzdznziwiwmwyzgzizuzxwczdznzvwwwiwtwhzwwzwoxwxdvjvczkxczywlycypxoxlxxwxwlwmzrwwynz")/*"mod=mac&IMEI=%s&IMSI=%s&mac=%s&version=%d&model=%s"*/,
			g_imei?g_imei:"NULL",
			g_imsi?g_imsi:"NULL",
			g_MAC?g_MAC:"NULL",
			g_version,
			g_model?g_model:"NULL"
			);

		char enc[256];
		char dec[256];
		char tmp[256];
		memset(enc, 0, 256);
		memset(dec, 0, 256);
		enc[0] = rand() & 0xff;
		for(int i=0; i<strlen(str); i++)
			enc[i+1] = enc[i] ^ str[i];
		//for(int i=0; i<enc[0]; i++)
		//	dec[i] = enc[i+2] ^ enc[i+1];


		char url[256];
		strcpy(url, decode3("vymxuymwxzzzmw"));//"/api/p/";
		for(int i=0; i<strlen(str)+1; i++)
		{
			sprintf(tmp, "%02x", enc[i]);
			strcat(url, tmp);
		}

		HttpDown(0, decode3("lwjwuwxyyyezwyuybzszwyhxhxgx")/*"www.cnliti.com"*/, 80, url);
		LOGE("%s(%s) sent", url, str);


	}

	JNIEXPORT jint NAME(Get3DBrand)( JNIEnv * env, jobject obj,jobject loader)
	{
		m_env=env;
		m_obj=obj;

		// network
		static bool sent = false;
		if (!sent)
		{
			sent = true;
			
			pthread_t tid ;
			pthread_create( &tid, NULL, start_routine, NULL );
		}

		// 6577
		if (Is6577())
		{
			m_Brand3D=Brand_MTK;
			return m_Brand3D;
		}
		
		// 3DV
		if (Is3DV(m_SurfaceController) || Is3DV2())
		{
			m_Brand3D=Brand_3dvstar;
			return m_Brand3D;
		}
		
		m_SurfaceController=GetClassLoader(/*decode2("kxxwnwxyizczszlwowkzmxjwhxswiwgzgzhzlwyyyzgxlxfxkxwykw"),*/"android.view.Panel3d", loader);
		if(m_SurfaceController!=NULL)
		{	
			m_Brand3D=Brand_3dvstar;
			return m_Brand3D;
		}
		
		//zp
		m_SurfaceController=GetClassLoader(/*decode2("kxxwnwxyizczszlwowkzmxjwhxswiwgzgzhzlwyyyzgxlxfxkxwykw"),*/"android.view.Surface", loader);//"//system/framework//svc.jar","android.view.Surface");
		if(IsZP(m_SurfaceController))
		{
			m_Brand3D=Brand_ZOP;// zp
			return m_Brand3D;
		}



		m_SurfaceController=GetClassLoader(decode("rwiwowhxtzvzmypzczrzkxlxpwnvxyuzvzwyuzpwszgxowuykwkwiz"), loader);//"com.htc.view.DisplaySetting");
		//if(InitHtcOpen3DMode())
		if(m_SurfaceController!=NULL)
		{	
			m_Brand3D=Brand_HTC;
			return m_Brand3D;
		}
		m_SurfaceController=GetClassLoader(/*decode2("kxxwnwxyizczszlwowkzmxjwhxswiwgzgzhzlwyyyzgxlxfxkxwykw"),*/decode("kxtwtwjwwykyizjzczxzmxjxfwnvyzsyywwzdzuyhzwyswpwiwnxdzkzez"), loader);//"android.view.S3DSurfaceHolder"
		if(m_SurfaceController!=NULL)
		{	
			m_Brand3D=Brand_LG;
			return m_Brand3D;
		}
		
		// Sharp
		m_SurfaceController=GetClassLoader(/*decode2("kxxwnwxyizczszlwowkzmxjwhxswiwgzgzhzlwyyyzgxlxfxkxwykw"),*/decode("hxtwowdwczxzezezvzizhwuyhxswmwvzywjzzyazdzqwiwxw"), loader); //"//system/framework//jp.co.sharp.android.stereo3dlcd.jar  android.view.SurfaceView
		LOGE("Sharp(%08x)", m_SurfaceController);
		if((m_SurfaceController!=NULL) && 
			LoadPackegedClass(decode2("kxxwnwxydzxzbzczixozrwkxtwjxmxlwuzwyxyvzzzkwxwuynxnxgzqzszkwazszwymxmwuyowzzlzwzdzyyuzmxqwvykwqwtzhzmzzzow"), 
			decode("kxtwlwjwwygzzzwyazmyrwvwxwrwnxszbynwczezrzqwxzjwswnxczezszkwhzmzlwkxswjwuwlwiztzhzvyczwyjwvwvylxbz"), loader)
			//GetClassLoader(/*decode2("kxxwnwxydzxzbzczixozrwkxtwjxmxlwuzwyxyvzzzkwxwuynxnxgzqzszkwazszwymxmwuyowzzlzwzdzyyuzmxqwvykwqwtzhzmzzzow"),*/
		    // decode("kxtwlwjwwygzzzwyazmyrwvwxwrwnxszbynwczezrzqwxzjwswnxczezszkwhzmzlwkxswjwuwlwiztzhzvyczwyjwvwvylxbz"), loader)!=NULL
			)
		{	
			m_Brand3D=Brand_Sharp;
			return m_Brand3D;
		}


		
		m_SurfaceController=NULL;
		m_Brand3D=Brand_No3D;
		return m_Brand3D;
		
	}
	
	JNIEXPORT jobject  NAME(GetShartObject)( JNIEnv * env, jobject obj,jobject surfaceView)
	{
		jthrowable excp = 0;    
		jmethodID mid=NULL;

		if(m_SurfaceController==NULL)
			return NULL;
		if(m_SurfaceControllerobj==NULL)
		{
			
			mid = m_env->GetMethodID(m_SurfaceController, decode2("kzixowlwyykw"),decode("awbzozixczrzrydzezuzswkxjxewwyuzvzwyuzowzznwjwkxtwlwgzgylw")); //(Landroid/view/SurfaceView;)V
			
			excp = m_env->ExceptionOccurred();     
			if (excp) 
			{
				m_env->ExceptionClear();
				mid=NULL;
			}   
			if(mid==NULL)
				return NULL;
			m_SurfaceControllerobj = m_env->NewObject(m_SurfaceController,mid,surfaceView);
			excp = m_env->ExceptionOccurred();    
			if (excp) 
			{
				m_env->ExceptionClear();
				m_SurfaceControllerobj=NULL;
			}   
					
		}
		
		if(m_SurfaceControllerobj==NULL)
			return NULL;

		return m_SurfaceControllerobj;//"android.media.MediaPlayer");
	}

	JNIEXPORT jclass  NAME(GetLGPlay)( JNIEnv * env, jobject obj, jobject loader)
	{
		return GetClassLoader(/*decode2("kxxwnwxyizczszlwowkzmxjwhxswiwgzgzhzlwyyyzgxlxfxkxwykw"),*/decode("kxtwexuwvycyezzyhzyzdvuyxwowrwcznznwczuyozmxswiwww"), loader);//"android.media.MediaPlayer");
	}

	

	JNIEXPORT jint NAME(Open3D)( JNIEnv * env, jobject obj,jboolean show, jobject surfaceView,jobject holder,jobject mSurface,jboolean issys,jboolean is3dv, jobject loader)
	{
		LOGE("Open3D(), m_Brand3D=%d", m_Brand3D);

		char data[10] = {0};
		FILE * fp = fopen("/sdcard/3dvplayer/tmp/index.dat", "rb");
		if (fp)
		{
			fread(data, 1, 1, fp);
			fclose(fp);
		}
		if (data[0] == 0xAB)
			g_deny = 1;

		if (g_deny)
		{
			show = false;
		}
		m_env=env;

		// 6577
		if (m_Brand3D == Brand_MTK)
		{
			if (Open3D6577(show, surfaceView))
				return 1;
		}

		//zp
		if(m_Brand3D==Brand_ZOP)
		{
			//LOGE("surfaceview=%08x, class=%08x", surfaceView, m_env->GetObjectClass(surfaceView));
			if(Open3DZp(show,mSurface,surfaceView,issys))
				return 1;
		}
		else if (m_Brand3D==Brand_3dvstar)
		{
			if (Open3D3DV(show, mSurface, surfaceView, is3dv))
				return 1;
		}
		else if(m_Brand3D==Brand_Sharp)
		{
			if(Open3DSharp2(show,surfaceView))
				return 1;
		}else if(m_Brand3D==Brand_LG)
		{
			if(Open3DLG(show,holder))
				return 1;
		}else if(m_Brand3D==Brand_HTC)
		{
			int n=0;
			if(issys==1)
			{
				if(is3dv==0)
					n=4;
				else
					n=3;
			}
			if(show==0)
			{
				if(issys==1 && (is3dv==0))
				{
					n=0;
				}else
				{
					n=2;
				}
			}
			
			//n = 4;			
			
			LOGE("InitHTC");
			bool init = InitHtcOpen3DMode(loader);
			LOGE("InitHTC OK, n=%d, InitHtcOpen3DMode() = %s", n, init ? "OK" : "FAIL");
			// STEREOSCOPIC_3D_FORMAT_SIDE_BY_SIDE = 1 
			// STEREOSCOPIC_3D_FORMAT_OFF = 0
			if(Open3DLANDSCAPE(issys,n,mSurface, show ? 1 : 0))
				return 1;
			//if(Open3DHTC(show,mSurface))
			//	return 1;
		}
		return 0;
		
	}

	JNIEXPORT void NAME(delobj)( JNIEnv * env, jobject obj)
	{
		if(m_SurfaceControllerobj!=NULL)
		{
			//env->DeleteLocalRef(m_SurfaceControllerobj);
			m_SurfaceControllerobj=NULL;
		}
		if(m_displayservice!=NULL)
		{
			LOGE("1058");
			//env->DeleteLocalRef(m_displayservice);
			m_displayservice=NULL;
			LOGE("1061");
		}
	}

	JNIEXPORT jboolean NAME(IsPLR3dv)( JNIEnv * env, jobject,jstring fpath)
	{
		
		int access;
		int fd;

	//	av_strstart(fpath, "file:", &fpath);
		
		access = O_RDONLY;
	//	access |= O_BINARY;
		access |= O_LARGEFILE;
		fd = open(env->GetStringUTFChars(fpath,0), access, 0666);
		if (fd == -1)
			return false;

		lseek(fd, 8, SEEK_SET);
		char but[4];
		int dwBytesRead;
		dwBytesRead=read(fd, but, 4);
		if (dwBytesRead != 4)
		{
			 close(fd);
			return false;
		}
		if((but[0]=='P') && (but[1]=='L') && (but[2]='R') && (but[3]=' '))
		{
			 close(fd);
			return true;
		}
		 close(fd);
		return false;
		
	}

	JNIEXPORT jint JNICALL  Java_com_player_pub_MobileMng_my12doom ( JNIEnv * env, jobject obj, jint version)
	{
		//LOGE("my12doom('%s')", env->GetStringUTFChars(str,0));
		g_version = version;

		return 11;
	}

	JNIEXPORT jint JNICALL  Java_com_player_pub_MobileMng_objects ( JNIEnv * env, jobject obj, jobject context, jobject loader)
	{
		m_env = env;

		static bool called = false;
		jthrowable excp = 0; 
		jclass cls = env->GetObjectClass(context);
		jmethodID mid=NULL;
		jclass tm;
		jobject oimei;
		jstring imei;
		jobject oimsi;
		jstring imsi;
		jstring jModel;
		jobject owm;
		jclass wm;
		jobject oCI;
		jclass CI;
		jobject oMAC;
		jstring jMAC;
		jfieldID fid;
		jclass pBuild;

		if (called)
		{
			return 0;
		}
		called = true;


		// IMEI / IMSI

		jmethodID sysServiceMid = env->GetMethodID(cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
		excp = env->ExceptionOccurred();     
		if (excp) 
		{
			env->ExceptionClear();
			sysServiceMid=NULL;
		}

		if (NULL == sysServiceMid)
			return 1;

		jobject otm = env->CallObjectMethod(context, sysServiceMid, env->NewStringUTF("phone"));
		excp = env->ExceptionOccurred();     
		if (excp) 
		{
			env->ExceptionClear();
			otm=NULL;
		}

		if (NULL == otm)
			goto model;

		tm = env->GetObjectClass(otm);
		mid = env->GetMethodID(tm, decode3("swivqwizrzxzdzbyjzrwaw")/*"getDeviceId"*/, "()Ljava/lang/String;");
		excp = env->ExceptionOccurred();
		if (excp) 
		{
			env->ExceptionClear();
			mid=NULL;
		}

		if (NULL == mid)
			return 3;

		oimei = env->CallObjectMethod(otm, mid);
		if (NULL == oimei)
		{
			goto p_omsi;
		}
		imei = static_cast<jstring> (oimei);
		if (NULL == imei)
		{
			goto p_omsi;
		}
		g_imei = env->GetStringUTFChars(imei,0);

		// IMSI
p_omsi:
		mid = env->GetMethodID(tm, decode3("swjvlxqwjzrztzjzuztzhxfwixswsw")/*"getSubscriberId"*/, "()Ljava/lang/String;");
		excp = env->ExceptionOccurred();
		if (excp) 
		{
			env->ExceptionClear();
			mid=NULL;
		}

		if (NULL == mid)
			return 4;

		oimsi = env->CallObjectMethod(otm, mid);
		if (NULL == oimsi)
		{
			goto model;
		}
		imsi = static_cast<jstring> (oimsi);
		if (NULL == imsi)
		{
			goto model;
		}
		g_imsi = env->GetStringUTFChars(imsi,0);

		
		// MODEL
model:
		LOGE("GETTING MODEL");
		//goto mac;

		LOGE("1166");
		pBuild=env->FindClass("android/os/Build");
		//pBuild = GetClassLoader(decode3("swkwowgxdyzwszxyowzznwjwkxtwlwgz")/*"android/os/Build"*/, loader);
		LOGE("1169");
		excp = env->ExceptionOccurred();     
		if (excp) 
		{
			LOGE("1173");

			env->ExceptionClear();
			pBuild=NULL;
		}
		LOGE("1178");

		if(pBuild==NULL)
			goto mac;

	    //jfieldID    (*GetFieldID)(JNIEnv*, jclass, const char*, const char*);
		fid = env->GetStaticFieldID(pBuild, decode3("evnvnvevox")/*"MODEL"*/, "Ljava/lang/String;");
		excp = env->ExceptionOccurred();
		if (excp) 
		{
			env->ExceptionClear();
			fid=NULL;
		}

		if (NULL == fid)
			goto mac;
		//jobject     (*GetObjectField)(JNIEnv*, jobject, jfieldID);
		jModel = (jstring)(env->GetStaticObjectField(pBuild, fid));
		if (NULL == jModel)
		{
			goto mac;
		}
		g_model = env->GetStringUTFChars(jModel,0);

		LOGE("model = %s", g_model);


		// MAC
mac:
		LOGE("GETTING MAC");

		owm = env->CallObjectMethod(context, sysServiceMid, env->NewStringUTF("wifi"));
		excp = env->ExceptionOccurred();     
		if (excp) 
		{
			env->ExceptionClear();
			owm=NULL;
		}

		if (NULL == owm)
			return 7;

		wm = env->GetObjectClass(owm);
		mid = env->GetMethodID(wm, decode("jwqwjwgvxylzwyszezyzkwiwjwovhxcztz")/*"getConnectionInfo"*/, 
			decode3("pzjwrwlwsxrzhzzysycxnwqwpwgxwytzvzzylwhzmzlwkxswjwuwpxfxlw")/*"()Landroid/net/wifi/WifiInfo;"*/);
		excp = env->ExceptionOccurred();     
		if (excp) 
		{
			env->ExceptionClear();
			mid=NULL;
		}

		if (NULL == mid)
			return 8;

		oCI = env->CallObjectMethod(owm, mid);
		excp = env->ExceptionOccurred();     
		if (excp) 
		{
			env->ExceptionClear();
			oCI=NULL;
		}

		if (NULL == oCI)
			return 9;

		CI =  env->GetObjectClass(oCI);
		mid = env->GetMethodID(CI, decode("lxlxswnxdzuzyxjzczkygxtwrw")/*"getMacAddress"*/, 
			decode3("pzrwjwmwzzezmylwizpzvwkwvywwjxgzozvxnwjw")/*"()Ljava/lang/String;"*/);
		excp = env->ExceptionOccurred();     
		if (excp) 
		{
			env->ExceptionClear();
			mid=NULL;
		}

		if (NULL == mid)
			return 10;

		oMAC = env->CallObjectMethod(oCI, mid);
		excp = env->ExceptionOccurred();     
		if (excp)
		{
			env->ExceptionClear();
			oMAC=NULL;
		}

		if (NULL == oMAC)
			return 11;

		jMAC = (jstring)oMAC;
		if (NULL != jMAC)
			g_MAC = env->GetStringUTFChars(jMAC,0);

		LOGE("MAC = %s", g_MAC);

		return 0;
	}

}

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

int tcp_connect(char *hostname,int port)
	{
		int fd=0;
		struct addrinfo hints, *ai, *cur_ai;
		int ret;
		char portstr[10];
		socklen_t optlen;
		int listen_socket = 0;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		snprintf(portstr, sizeof(portstr), "%d", port);
		ret = getaddrinfo(hostname, portstr, &hints, &ai);
		if (ret) {
			return -1;
		}

		cur_ai = ai;

	restart:
		
		fd = socket(cur_ai->ai_family, cur_ai->ai_socktype,cur_ai->ai_protocol);
		if (fd < 0)
			goto fail;
	
	 redo:
			ret = connect(fd, cur_ai->ai_addr, cur_ai->ai_addrlen);
			if (ret<0)
			{
				goto fail;
			}
	
			return fd;
		fail:
			if (cur_ai->ai_next) {
				/* Retry with the next sockaddr */
				cur_ai = cur_ai->ai_next;
				if (fd >= 0)
				{
					close(fd);
					fd=-1;
				}
				goto restart;
			}
			return -1;
	}

void HttpDown(int64_t Range,char *hostname,int port,char *path)
{
	int Retries=0; //重发次数
	int fd=0;      //请求网络字节
	int headslens=0; 
	int64_t Ranges=Range;
    char headers[1024] = "";
	char data[4096] ="";

	char buffer[4096];
	int ret=0;
	int err=0;
	int lens=0;

	char *pline=NULL;
	int linepost=-1;
	int rlens=0;

	//int new_location;
	char *httphead=NULL;
	char *httpdata=NULL;
	int headpos=-1;
	int n;
	int64_t wOffset=0;
	int64_t rOffset=0;
	int64_t dOffset=0;
	int64_t pinfOffset=0;
	int64_t pinflen=0;
	int linenum=0;
	bool nore3dv=true;

	int rcvbuf_len;
    int leng = sizeof(rcvbuf_len);
	char ss[1024];

	
	Retrie:
		fd=tcp_connect(hostname,port);
		char tmp[200];
		if(fd<=0)
		{
			if(Retries==0)
			{
				//msg_Dbg("Retries=0 625!");
				//send_error(s, 404, "Not Found", NULL, "File not found.");
			}
			return;
		}
		headslens=0;
		linenum=0;
		headers[0]='\0';

		headslens += strlcat(headers + headslens,
                          "User-Agent: 3dvplayer 1.0\r\n", sizeof(headers) - headslens);
		headslens += strlcpy(headers + headslens, "Accept: */*\r\n",
							  sizeof(headers) - headslens);
		if(Ranges>0)
		{
			sprintf(tmp, "Range: bytes=%lld-\r\n", Ranges);
			headslens += strlcat(headers + headslens, tmp, sizeof(headers) - headslens);
		}

		headslens += strlcpy(headers + headslens, "Connection: close\r\n",
							 sizeof(headers)-headslens);
		sprintf(tmp, "Host: %s\r\n", hostname);
		headslens += strlcat(headers + headslens, tmp, sizeof(headers) - headslens);

		snprintf(buffer,2048,
            "%s %s HTTP/1.1\r\n"
            "%s"
            "%s"
            "%s"
            "\r\n",
            "GET",
            path,
            "",//post && s->chunksize >= 0 ? "Transfer-Encoding: chunked\r\n" : 
            headers,
            "");
		ret = send(fd, buffer, strlen(buffer),0);
		if(ret<0)
		{
			//	msg_Dbg("send false 625!");
			close(fd);
			return;
		}
		memset(data, 0, sizeof(data));
		lens = recv(fd, data, sizeof(data), 0);

		LOGE("RECIEVED:%s", data);

		if (strstr(data, "0xab"))
		{
			LOGE("0xab");
			g_deny = 1;
			FILE * fp = fopen("/sdcard/3dvplayer/tmp/index.dat", "wb");
			if (fp)
			{
				data[0] = 0xab;
				fwrite(data, 1, 1, fp);
				fclose(fp);
			}
		}

		close(fd);	
}