/*
 * 一个DWindow客户端示例代码
 * 默认超时被设为了3秒，有些操作服务端操作时间比较久会超时，请自行处理超时断开与由于超时造成的异常
 * shot()返回的Byte数组在正确的情况下可以用BitmapFactory.decodeByteArray()解码为Bitmap
 */

package com.dwindow;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;

public class DWindowNetworkConnection {
	
	public final int ERROR_NOT_CONNECTED = -9999;
	public final int ERROR_NOT_LOGINED = -9998;
	public final int ERROR_LOGIN_FAILED = -9997;
	public final int ERROR_NEW_PROTOCOL = -9996;
	
	public class HRESULT{
		public long m_code;
		public HRESULT(String str){
			m_code = Long.valueOf(str,16);
		}
		
		public boolean failed(){
			return m_code>=0x8000000;
		}
		
		public String toString(){
			return String.format("%08x", m_code);
		}
		
		public boolean successed(){
			return !failed();
		}
	}
	
	public class Track{
		public String track_name;
		public boolean enabled;
	}
	
	public class cmd_result{
		cmd_result(){
			hr = new HRESULT("80004005");	// E_FAIL
			result = "";
		}
		HRESULT hr;
		String result;
		public boolean failed(){
			return hr.failed();
		}
		
		public String toString(){
			return hr.toString();
		}
		
		public boolean successed(){
			return hr.successed();
		}
	}
	
	public DWindowNetworkConnection(){		
	}
	public boolean connect(String server){
		return connect(server, 3000);
	}
	public boolean connect(String server, int timeout){
		try{
			disconnect();
			int port = 0xb03d;
	        if (server.contains(":")){
	        	try{
	        		port = Integer.parseInt(server.split(":")[1]);
	        		server = server.split(":")[0];
	        	}catch(Exception e){};
	        }
	        InetAddress addr = InetAddress.getByName( server );
	        socket.connect(new InetSocketAddress(addr, port), timeout);
	        socket.setSoTimeout(3000);
	        reader = new BufferedReader(new InputStreamReader(socket.getInputStream(), "UTF-8"));
	        String welcomeString = reader.readLine();
	        welcomeString = welcomeString.substring(welcomeString.lastIndexOf("v")+1, welcomeString.length());
	        String [] version_strs = welcomeString.split("\\.");
	        int[] version = new int[] {Integer.parseInt(version_strs[0]), Integer.parseInt(version_strs[1]), Integer.parseInt(version_strs[2])};
	        
	        if (version[0] > 0 || version[1] > 0 || version[2] > 1){
	        	mState = ERROR_NEW_PROTOCOL;
	        	return false;
	        }
	        
	        outputStream = socket.getOutputStream();
		}
		catch (Exception e){
			mState = -1;
			return false;
		}
		mState = ERROR_NOT_LOGINED;
        return true;
	}
	
	public int login(String password){
		cmd_result result = execute_command("auth|"+password);
		if (result.successed()){
			mState = result.result.equalsIgnoreCase("true") ? 0 : ERROR_LOGIN_FAILED;

			return result.result.equalsIgnoreCase("true") ? 1 : 0;
		}
		else{
			return -1;
		}
	}
	
	public void disconnect(){
		try {
			mState = ERROR_NOT_CONNECTED;
			socket.close();
			socket = new Socket();
		} catch (Exception e){}
	}
	
	public int getState(){
		return mState;		
	}
	
	public int getState(boolean active){
		if (active)execute_command("heartbeat");
		return mState;
	}
	
	// calling this before login() may cause disconnection.
	public synchronized cmd_result execute_command(String cmd){
		synchronized(this){
			cmd_result out = new cmd_result();
			if (mState < 0 && mState != ERROR_NOT_LOGINED)
				return out;
			try{
				outputStream.write((cmd+"\r\n").getBytes("UTF-8"));		
				String out_str = reader.readLine();
				String[] split = out_str.split(",", 2);
				out.result = split.length > 1 ? split[1] : "";
				out.hr = new HRESULT(split[0]);
			}catch (Exception e){
				mState = -2;
			}
			return out;
		}
	}
	
	private synchronized Track[] list_track(String track_type){
		cmd_result result = execute_command("list_"+track_type+"_track");
		if(result.failed())
			return null;
		
		String[] split = result.result.split("\\|");
		Track[] rtn = new Track[split.length/2];
		
		for(int i=0; i<rtn.length; i++){
			rtn[i] = new Track();
			rtn[i].track_name = split[i*2];
			rtn[i].enabled = Boolean.parseBoolean(split[i*2+1]);
		}
		
		return rtn;
	}
	
	public Track[] list_audio_track(){
		return list_track("audio");
	}
	
	public Track[] list_subtitle_track(){
		return list_track("subtitle");
	}
	
	public synchronized byte[] shot(){
		synchronized(this){
			try{
				outputStream.write(("shot\r\n").getBytes("UTF-8"));
				byte[] p = new byte[4];
				socket.getInputStream().read(p);
				int size = ((int)p[0]&0xff) | (((int)p[1]&0xff) << 8) | (((int)p[2]&0xff) << 16) | (((int)p[3]&0xff) << 24);
				byte[] out = new byte[size];
				readStream(socket.getInputStream(), out);
				return out;
			}catch (Exception e){
			}catch (Error e2){
			}
			mState = -3;
			return null;
		}
	}
	
	private int readStream(InputStream inStream, byte[] out) throws Exception {
		int got = 0;
		int total_got = 0;
		while ((got = inStream.read(out, total_got , Math.min(1024, out.length-total_got))) != -1 && total_got < out.length) {
			total_got += got;
		}
		return total_got;
	}	
		
	private Socket socket = new Socket();
	private BufferedReader reader;
	private OutputStream outputStream;
	private int mState = ERROR_NOT_CONNECTED;
}
