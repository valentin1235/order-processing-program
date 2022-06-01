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
5. `orders.json` 파일에 명시된 `prepTime`만큼 기다리면 주문이 처리됩니다 
#### 배달원 생성 프로그램
1. 배달원 생성 프로그램 폴더로 이동합니다 `cd ./courier_generator`
2. 배달원 생성 프로그램을 컴파일 합니다 `clang -std=c89 -W -Wall -pedantic-errors *.c`
3. 배달원 생성 프로그램을 실생합니다 `./a.out 127.0.0.1 3000`
4. 주문이 특정된 배달원의 경우 아래와 같이 입력합니다.
```
Banana#10 /* Banana라는 주문 만 픽업할 예정이고, 도착하는데 10초걸림 */
```
5. 주문이 특정되지 않은 배달원의 경우 아래와같이 입력합니다
```
none#10 /* 아무 주문이나 다 픽업 가능하고, 도착하는데 10초걸림 */
```

## 영상
- [LINK](https://youtu.be/7DalTaIuk_E](https://youtu.be/sikZwmxh7nU))

## 기능 및 플로우
### < 주문 처리하기 >
1. `/order_process_server`에서 프로그램을 실해알때 등록한 `orders.json` 파일을 읽어서 주문 하나당 하나의 스래드를 생성합니다 `process_orders`
2. 주문 처리 스레드`cook` 에서는 주문에 해당되는 `prepTime`만큼 프로세스를 `sleep`하고 주문 처리가 완료되면 배달원에게 주문을 넘기고(`deliver_order`), 배달원이 없다면 주문목록 배열`g_ready_orders`에 넣습니다
### < 배달원 등록 하기 >
1. `/courier_generator`에서 배달원을 등록 요청을 보내고, `/order_process_server`에 배달원이 등록되면 배달원 처리 스레드`process_courier_thread`로 이동합니다
2. 입력받은 배달원의 도착시간만큼 스레드를 `sleep` 합니다
3. 배달원의 주문이 특정되어 있으면 "특정 주문만 픽업하는 배달원 배열"`s_target_couriers`에 들어갑니다
4. "아무주문이나 픽업 가능한 배달원"이면 `s_random_courier_queue`에 들어갑니다
5. 배달원 등록이 완료되고, 주문목록에 처리할수 있는 주문이 있으면 `s_random_courier_queue`에서 배달원 하나를 꺼내서 주문을 처리합니다
### < 주문이 특정된 배달원의 배달처리 >
1. `/order_process_server`에서 프로그램을 시작할때 "주문이 특정된 배달원이 주문을 찾는 스레드"`listen_target_delivery_event_thread`를 생성합니다
2. 이 스레드에서는 `g_ready_orders`와 `s_target_couriers`를 계속 감시하면서 배달원에게 주어진 특정 주문이 매칭될때마다 배달이벤트를 수행합니다

## 이슈로그
### < 캡슐화 이슈 >
##### 상황
- c는 oop언어가 아닌 관계로 클래스를 통한 캡슐화와 불가능한 상황
- 데이터가 오픈되어 있어서 실수할 여지가 생김
##### 해결
- 파일하나를 하나의 클래스단위로 정하도록 해서 파일자체에서 static을 사용해서 데이터를 캡슐화 한다
- 함수 또한 static함수로 만들어서 private 함수로 만든다

### < SIGPIE 이슈 >
##### 상황
- 클라이언트가 서버에 접속해있다가 일방적으로 연결을 끊는 즉시에 대부분의 경우는 errno가 설정되지 않는다. 그리고 서버에서 열려있는 클라이언트에 read동작을 할 때 errno가 ECONNRESET(connection reset by peer)로 설정이 된다
- 하지만 어쩌다 한번 씩 끊어진 클라이언트 소켓에 read를 하기전에 errno가 EPIPE(broken pipe)로 설정이 되고 서버가 강제로 종료되어 버린다.
##### 원인
- 리눅스 운영체제에서 소켓연결이 끊어지고, 프로그램에서 끊어진 소켓에 데이터를 쓰거나 읽으려 하면 SIGPIPE 시그널을 발생시킨다. 특별한 설정이 없으면 프로세스는 SIGPIPE시그널을 받으면 프로그램을 종료시킨다.
##### 해결
- `signal(SIGPIPE, SIG_IGN);` 를 프로그램 시작 직후에 넣어주어서 운영체제에서 SIGPIPE 시그널을 무시하도록 한다

### < 프로세스 종료 강제 종료 이슈 >
##### 상황
- `ctrl + c` 를 통해서 프로세스를 강제종료 할때 생성되어있는 자식 스레드를 종료해주지 못한다
##### 해결
- signal(SIGINT, SIGINT_handler); 를 통해서 프로세스를 종료하는 시그널이 들어왔을 때 처리하는 헨들러를 등록해서 프로세스가 종료될때 자식스레드도 전부 종료하도록 한다
- 시그널 핸들러 쪽에서 스레드에 접근하기 위해서 모든 스레드는 전역변수로 처리해주도록 한다

### < 주문조회 락 이슈 >
##### 상황
- 주문이 등록되고, prepTime이 종료되면 주문목록에 추가된다
- 배달 타겟이 정해진 배달원은 주문목록을 while문을 돌면서 특정 배달명이 있는지 계속 보고있다가 해당 주문이 들어오면 주문을 목록에서 빼서 처리한다
- 이때 주문목록에서 정해진 주문명이 없으면 프로그램이 멈춘다
##### 원인 
- 주문목록에서 주문을 읽을때 mutex lock을 거는데 주문목록에 찾고자 하는 주문이 없으면 mutex unlock을 하기전에 값을 return 해서 다음 주문목록 조회할때 무한하게 wait하는 상태에 들어간다
##### 해결
- goto문을 사용해서 실패하면 mutex unlock으로 이동하도록 변경해서 해결
