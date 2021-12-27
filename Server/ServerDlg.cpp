
// ServerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"

#include <ctime>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <fstream>
#include <string>
#include <curl/curl.h>

using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CServerDlg dialog



CServerDlg::CServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERVER_DIALOG, pParent)
	, m_msgString(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BOXCHAT, m_msgString);
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_SOCKET, SockMsg)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_LISTEN, &CServerDlg::OnBnClickedListen)
	ON_BN_CLICKED(IDC_CANCEL, &CServerDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_CLONE, &CServerDlg::OnBnClickedClone)
END_MESSAGE_MAP()


// CServerDlg message handlers

BOOL CServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	HICON hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON1));
	SetIcon(hIcon, FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CServerDlg::Split(CString src, CString des[3])
{
	int p1, p2, p3;
	p1 = src.Find(_T("\r\n"), 0);
	des[0] = src.Mid(0, p1);
	p2 = src.Find(_T("\r\n"), p1 + 1);
	des[1] = src.Mid(p1 + 2, p2 - (p1 + 2));
	p3 = src.Find(_T("\r\n"), p2 + 1);
	if(p3 != -1)
		des[2] = src.Mid(p2 + 2, p3 - (p2 + 2));
}

char* CServerDlg::ConvertToChar(const CString& s)
{
	int nSize = s.GetLength();
	char* pAnsiString = new char[nSize + 1];
	memset(pAnsiString, 0, nSize + 1);
	wcstombs(pAnsiString, s, nSize + 1);
	return pAnsiString;
}

void CServerDlg::mSend(SOCKET sk, CString Command)
{
	int Len = Command.GetLength();
	Len += Len;
	PBYTE sendBuff = new BYTE[1000];
	memset(sendBuff, 0, 1000);
	memcpy(sendBuff, (PBYTE)(LPCTSTR)Command, Len);
	send(sk, (char*)&Len, sizeof(Len), 0);
	send(sk, (char*)sendBuff, Len, 0);
	delete sendBuff;
}

int CServerDlg::mRecv(SOCKET sk, CString& Command)
{
	PBYTE buffer = new BYTE[1000];
	memset(buffer, 0, 1000);
	recv(sk, (char*)&buffLength, sizeof(int), 0);
	recv(sk, (char*)buffer, buffLength, 0);
	TCHAR* ttc = (TCHAR*)buffer;
	Command = ttc;

	if (Command.GetLength() == 0)
		return -1;
	return 0;
}

void CServerDlg::OnBnClickedListen()
{
	// TODO: Add your control notification handler code here

	UpdateData();
	sockServer = socket(AF_INET, SOCK_STREAM, 0);
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_port = htons(PORT);
	serverAdd.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(sockServer, (SOCKADDR*)&serverAdd, sizeof(serverAdd));
	listen(sockServer, 5);
	int err = WSAAsyncSelect(sockServer, m_hWnd, WM_SOCKET, FD_READ | FD_ACCEPT | FD_CLOSE);
	if (err)
		MessageBox((LPCTSTR)"Cannot call WSAAsyncSelect");
	GetDlgItem(IDC_LISTEN)->EnableWindow(FALSE);
	number_Socket = 0;
	pSock = new SockName[200];
	srand((unsigned)time(NULL));
	R = rand();
}


void CServerDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

std::wstring get_utf16(const std::string& str)
{
	if (str.empty()) return std::wstring();
	int sz = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), 0, 0);
	std::wstring res(sz, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &res[0], sz);
	return res;
}

LRESULT CServerDlg::SockMsg(WPARAM wParam, LPARAM lParam)
{
	if (WSAGETSELECTERROR(lParam))
	{
		// Display the error and close the socket
		closesocket(wParam);
	}
	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_ACCEPT:
		{
			pSock[number_Socket].sockClient = accept(wParam, NULL, NULL);
			GetDlgItem(IDC_LISTEN)->EnableWindow(FALSE);
			break;
		}
	case FD_READ:
		{
			int post = -1, dpos = -1;
			for (int i = 0; i < number_Socket; i++) {
				if (pSock[i].sockClient == wParam) {
					if (i < number_Socket)
						post = i;
				}
			}

			CString temp;
			if (mRecv(wParam, temp) < 0)
				break;
			Split(temp, strResult);
			int flag = _ttoi(strResult[0]);
			char* user = ConvertToChar(strResult[1]);
			char* pass = ConvertToChar(strResult[2]);
			switch (flag)
			{
			case 1://Login
				{
					int t = 0;
					if (number_Socket > 0) {
						for (int i = 0; i < number_Socket; i++) {
							if (strcmp(user, pSock[i].Name) == 0)//Trung ten
							{
								t = 1;
								break;
							}
						}
					}
					if (t != 1) {
						//Kiem tra file accounts.json
						Json::Value root;
						Json::Reader reader;
						ifstream input_file("Accounts.json");
						if (!reader.parse(input_file, root)) {
							t = 1;
						}
						input_file.close();
						bool found_user = false, found_pass = false;
						for (int index = 0; index < root.size(); index++) {
							if (root[index]["username"].asString() == user) {
								found_user = true;
								if (root[index]["password"].asString() == pass)
									found_pass = true;
								break;							
							}
						}
						if (!found_user) {
							t = 2;
						}
						else {
							if (!found_pass)
								t = 3;
						}
					}
					//Gui ket qua ve client
					if (t == 0) {
						strcpy(pSock[number_Socket].Name, user);
						Command = _T("1\r\n1\r\n");
						m_msgString += strResult[1] + _T(" login\r\n");
						UpdateData(FALSE);
						number_Socket++;
					}					
					else if(t == 2)
						Command = _T("1\r\n2\r\n");
					else if (t == 3)
						Command = _T("1\r\n3\r\n");
					else 
						Command = _T("1\r\n0\r\n");
					mSend(wParam, Command);
					UpdateData(FALSE);
					break;
				}
			case 2:
				{
					int t = 0;
					if (number_Socket > 0) {
						for (int i = 0; i < number_Socket; i++) {
							if (strcmp(user, pSock[i].Name) == 0)//Trung ten
							{
								t = 1;
								break;
							}
						}
					}
					//Kiem tra file Accounts.json de xem username da ton tai chua
					Json::Value root;
					Json::Reader reader;
					ifstream input_file("Accounts.json");
					if (!reader.parse(input_file, root)) {
						t = 1;
					}
					input_file.close();
					bool found = false;
					for (int index = 0; index < root.size(); index++) {
						if (root[index]["username"].asString() == user) {
							found = true;
							break;
						}
					}
					if (found) {
						t = 2;
					}
					else {
						Json::Value item;
						item["username"] = user;
						item["password"] = pass;
						root.append(item);
						ofstream output_file;
						output_file.open("Accounts.json");
						Json::StyledStreamWriter writer;
						writer.write(output_file, root);
						output_file.close();
					}
					//Gui ket qua ve client
					if (t == 0) {
						strcpy(pSock[number_Socket].Name, user);
						Command = _T("2\r\n1\r\n");
						m_msgString += strResult[1] + _T(" sign in\r\n");
						UpdateData(FALSE);
						number_Socket++;
					}
					else if (t == 2)
						Command = _T("2\r\n2\r\n");
					else
						Command = _T("2\r\n0\r\n");
					mSend(wParam, Command);
					UpdateData(FALSE);
					break;
				}
			case 3:
				{
					m_msgString += pSock[post].Name;
					m_msgString += " logout \r\n";
					closesocket(wParam);
					for (int j = post; j < number_Socket; j++) {
						pSock[post].sockClient = pSock[post + 1].sockClient;
						strcpy(pSock[post].Name, pSock[post + 1].Name);
					}
					number_Socket--;
					UpdateData(FALSE);
					break;
				}
			case 4: 
				{
					//Get date time from client
					CT2CA pszConvertedAnsiString(strResult[1]);
					string dateTime(pszConvertedAnsiString);
					string file_name = "World " + dateTime + ".json";
					//Check if data is available
					Json::Value root;
					Json::Reader reader;
					ifstream infile(file_name);
					bool exist = false;
					if (infile.good())
						exist = true;
					if (exist) {
						if (!reader.parse(infile, root)) {
							t = 1;
						}
						else {
							for (int index = 0; index < root.size(); index++) {
								if (root[index]["country"].asString() == "World") {
									t = root[index]["cases"].asInt();
									break;
								}
							}
						}
					}
					else
						t = 0;
					infile.close();
					m_msgString += pSock[post].Name;
					m_msgString += " tra cuu World ";
					m_msgString += strResult[1];
					m_msgString += "\r\n";

					CString cases;
					cases.Format(L"%d", t);
					Command = "4\r\n";
					Command += cases;
					Command += "\r\n";
					
					mSend(wParam, Command);
					UpdateData(FALSE);
					break;
				}
			case 5:
				{
					//Get date time from client
					CT2CA pszConvertedAnsiString(strResult[1]);
					string dateTime(pszConvertedAnsiString);
					string file_name = "World " + dateTime + ".json";
					//Check if data is available
					Json::Value root;
					Json::Reader reader;
					ifstream infile(file_name);
					bool exist = false;
					if (infile.good())
						exist = true;
					if (exist) {
						if (!reader.parse(infile, root)) {
							t = 1;
						}
						else {
							for (int index = 0; index < root.size(); index++) {
								if (root[index]["country"].asCString() == strResult[2]) {
									t = root[index]["cases"].asInt();
									break;
								}
							}
						}
					}
					else
						t = 0;
					infile.close();
					m_msgString += pSock[post].Name;
					m_msgString += " tra cuu ";
					m_msgString += strResult[2] + _T(" ");
					m_msgString += strResult[1];
					m_msgString += "\r\n";

					CString cases;
					cases.Format(L"%d", t);
					Command = "5\r\n";
					Command += cases;
					Command += "\r\n";
				
					mSend(wParam, Command);
					UpdateData(FALSE);
					break;
				}
			case 6:
				{
					//Get date time from client
					CT2CA pszConvertedAnsiString(strResult[1]);
					string dateTime(pszConvertedAnsiString);
					string file_name = "World " + dateTime + ".json";
					//Check if data is available
					Json::Value root;
					Json::Reader reader;
					ifstream infile(file_name);
					bool exist = false;
					if (infile.good())
						exist = true;
					if (exist) {
						if (!reader.parse(infile, root)) {
							t = 1;
						}
						else {
							for (int index = 0; index < root.size(); index++) {
								if (root[index]["country"].asString() == "Vietnam") {
									t = root[index]["cases"].asInt();
									break;
								}
							}
						}
					}
					else
						t = 0;
					infile.close();
					m_msgString += pSock[post].Name;
					m_msgString += " tra cuu VN- Ca nuoc ";
					m_msgString += strResult[1];
					m_msgString += "\r\n";

					CString cases;
					cases.Format(L"%d", t);
					Command = "6\r\n";
					Command += cases;
					Command += "\r\n";

					mSend(wParam, Command);
					UpdateData(FALSE);
					break;
				}
			case 7:
				{
					//Get date time from client
					CT2CA pszConvertedAnsiString(strResult[1]);
					string dateTime(pszConvertedAnsiString);
					CT2CA pszConvertedAnsiString_2(strResult[2]);
					string province_name(pszConvertedAnsiString_2);
					string file_name = "VN " + dateTime + ".json";
					//Check if data is available
					Json::Value root;
					Json::Reader reader;
					ifstream infile(file_name);
					bool exist = false;
					if (infile.good())
						exist = true;
					if (exist) {
						if (!reader.parse(infile, root)) {
							t = 1;
						}
						else {
							if (strResult[2] == _T("B?"))
								t = root[u8"B\u0110"]["cases"].asInt();
							else if (strResult[2] == _T("?N"))
								t = root[u8"\u0110N"]["cases"].asInt();
							else if (strResult[2] == _T("?G"))
								t = root[u8"\u0110G"]["cases"].asInt();
							else if (strResult[2] == _T("?B"))
								t = root[u8"\u0110B"]["cases"].asInt();
							else
								t = root[province_name]["cases"].asInt();
						}
					}
					else
						t = 0;
					infile.close();
					m_msgString += pSock[post].Name;
					m_msgString += " tra cuu VN- ";
					m_msgString += strResult[2] + _T(" ");
					m_msgString += strResult[1];
					m_msgString += "\r\n";

					CString cases;
					cases.Format(L"%d", t);
					Command = "7\r\n";
					Command += cases;
					Command += "\r\n";

					mSend(wParam, Command);
					UpdateData(FALSE);
					break;
				}
			}
			break;
		}
	case FD_CLOSE:
		{
			UpdateData();
			int post = -1;
			for (int i = 0; i < number_Socket; i++) {
				if (pSock[i].sockClient == wParam) {
					if (i < number_Socket)
						post = i;
				}
			}
			m_msgString += pSock[post].Name;
			m_msgString += " logout\r\n";
			closesocket(wParam);
			for (int j = post; j < number_Socket; j++) {
				pSock[post].sockClient = pSock[post + 1].sockClient;
				strcpy(pSock[post].Name, pSock[post + 1].Name);
			}
			number_Socket--;
			UpdateData(FALSE);
			break;
		}
	}
	return 0;
}

size_t writeFunction(void* ptr, size_t size, size_t nmemb, string* data) {
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

void CServerDlg::OnBnClickedClone()
{
	// TODO: Add your control notification handler code here

	time_t t = time(0);   // get time now
	tm* now = localtime(&t);

	//Create json variables
	Json::Value root, check;
	Json::Reader reader;
	Json::StyledStreamWriter writer;
	//Create file's name
	string date = to_string(now->tm_mday) + "-" + to_string(now->tm_mon + 1) + "-" + to_string(now->tm_year + 1900);
	string file_name = "World (" + date + ").json";
	cout << file_name << endl;

	//Check if current date already has json file
	ifstream infile(file_name);
	bool exist = false;
	bool allow_clone = true;
	if (infile.good())
		exist = true;
	if (exist) {
		if (!reader.parse(infile, check)) {
			MessageBox(_T("Access-file error"), _T("ERROR"), 0);
		}
		else {
			int size = check.size() - 1;
			if (check[size]["hour"].asInt() == now->tm_hour) {
				if (check[size]["minute"].asInt() >= now->tm_min - 30)
					allow_clone = false;
			}
			if (check[size]["hour"].asInt() == (now->tm_hour - 1)) {
				if (check[size]["minute"].asInt() >= now->tm_min + 30)
					allow_clone = false;
			}
		}
	}
	infile.close();

	//Processing json file
	ofstream output_file;
	if (!allow_clone) {
		MessageBox(_T("Already cloned in nearest 30 minutes"), _T("ERROR"), 0);
	}
	else {
		string response_string;
		string header_string;
		//HTTP GET request
		curl_global_init(CURL_GLOBAL_DEFAULT);
		auto curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, "https://coronavirus-19-api.herokuapp.com/countries");
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
			curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

			curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			curl = NULL;
		}
		//Parse data into root
		if (!reader.parse(response_string, root)) {
			MessageBox(_T("Get-data error"), _T("ERROR"), 0);
		}
		else {
			Json::Value current_time;
			current_time["hour"] = now->tm_hour;
			current_time["minute"] = now->tm_min;
			root.append(current_time);

			output_file.open(file_name);
			writer.write(output_file, root);
			output_file.close();

			MessageBox(_T("Clone successfully"), 0);

			date.insert(0, "(");
			date.append(")");
			string s_time = "(" + to_string(now->tm_hour) + ":" + to_string(now->tm_min) + ")";
			m_msgString += _T("Clone du lieu thanh cong ");
			m_msgString += date.c_str();
			m_msgString += s_time.c_str();
			m_msgString += "\r\n";
		}
		UpdateData(FALSE);
	}
}
