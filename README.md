## 스펙
- c89
- Apple clang version 13.1.6 (clang-1316.0.21.2.3)
- Target: x86_64-apple-darwin21.4.0

## 서비스
- 배달원 생성 프로그램 `/courier_generator`
- 주문 처리 프로그램 `/order_process_server`

## 실행하기
#### 주문 처리 프로그램
1. 주문 처리 프로그램 폴더로 이동합니다 `cd ./order_process_server`
2. 주문 처리 프로그램을 컴파일 합니다 `clang -std=c89 -W -Wall -pedantic-errors *.c`
3. 주문 파일(`orders.json`)이 있는지 확인합니다
4. 주문 처리 프로그램을 실행합니다 `./a.out orders.json`
#### 배달원 생성 프로그램
1. 배달원 생성 프로그램 폴더로 이동합니다 `cd ./courier_generator`
2. 배달원 생성 프로그램을 컴파일 합니다 `clang -std=c89 -W -Wall -pedantic-errors *.c`
3. 배달원 생성 프로그램을 실생합니다 `./a.out 127.0.0.1 3000`

## 영상
[LINK](https://youtu.be/7DalTaIuk_E](https://youtu.be/sikZwmxh7nU))

## 이슈로그
### 캡슐화 이슈
##### 상황
- c는 oop언어가 아닌 관계로 클래스를 통한 캡슐화와 불가능한 상황
- 데이터가 오픈되어 있어서 실수할 여지가 생김
##### 해결
- 파일하나를 하나의 클래스단위로 정하도록 해서 파일자체에서 static을 사용해서 데이터를 캡슐화 한다
- 함수 또한 static함수로 만들어서 private 함수로 만든다

### SIGPIE 이슈
##### 상황
- 클라이언트가 서버에 접속해있다가 일방적으로 연결을 끊는 즉시에 대부분의 경우는 errno가 설정되지 않는다. 그리고 서버에서 열려있는 클라이언트에 read동작을 할 때 errno가 ECONNRESET(connection reset by peer)로 설정이 된다
- 하지만 어쩌다 한번 씩 끊어진 클라이언트 소켓에 read를 하기전에 errno가 EPIPE(broken pipe)로 설정이 되고 서버가 강제로 종료되어 버린다.
##### 원인
- 리눅스 운영체제에서 소켓연결이 끊어지고, 프로그램에서 끊어진 소켓에 데이터를 쓰거나 읽으려 하면 SIGPIPE 시그널을 발생시킨다. 특별한 설정이 없으면 프로세스는 SIGPIPE시그널을 받으면 프로그램을 종료시킨다.
##### 해결
- `signal(SIGPIPE, SIG_IGN);` 를 프로그램 시작 직후에 넣어주어서 운영체제에서 SIGPIPE 시그널을 무시하도록 한다
