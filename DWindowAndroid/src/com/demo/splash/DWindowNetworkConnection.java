package com.demo.splash;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;

public class DWindowNetworkConnection {
	
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
			int port = 8080;
	        if (server.contains(":")){
	        	try{
	        		port = Integer.parseInt(server.split(":")[1]);
	        		server = server.split(":")[0];
	        	}catch(Exception e){};
	        }
	        InetAddress addr = InetAddress.getByName( server );
	        socket.connect(new InetSocketAddress(addr, port), timeout);
	        reader = new BufferedReader(new InputStreamReader(socket.getInputStream(), "UTF-8"));
	        String welcomeString = reader.readLine();
	        
	        outputStream = socket.getOutputStream();
		}
		catch (Exception e){
			mState = -1;
			return false;
		}
        return true;
	}
	public int login(String password){
		cmd_result result = execute_command("auth|"+password);
		if (result.successed())
			return result.result.equalsIgnoreCase("true") ? 1 : 0;
		else
			return -1;
	}
	public void disconnect(){
		try {
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
	public cmd_result execute_command(String cmd){
		cmd_result out = new cmd_result();
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
	
	public byte[] shot(){
		try{
			outputStream.write(("shot\r\n").getBytes("UTF-8"));
			byte[] p = new byte[4];
			socket.getInputStream().read(p);
			int size = ((int)p[0]&0xff) | (((int)p[1]&0xff) << 8) | (((int)p[2]&0xff) << 16) | (((int)p[3]&0xff) << 24);
			byte[] out = new byte[size];
			readStream(socket.getInputStream(), out);
			return out;
		}catch (Exception e){
		}
		mState = -3;
		return null;		
	}
	
	public int readStream(InputStream inStream, byte[] out) throws Exception {
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
	private int mState = 0;
}
