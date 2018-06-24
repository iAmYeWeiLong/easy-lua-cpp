
#-*-coding:utf-8-*-
import gevent

def kill():
	asyncWatcher.stop()
	#asyncWatcher.send()

def getAoi():
	print 'callll '

	'''
	while True:#因为多次唤醒,会表现为一次真正醒来,所以要while True
		iServiceSource,iMsgType,sMsg=easyLua.getMsg()  #iReqestId,  # 不阻塞
		if not iServiceSource:
			return
	'''


def onAoiAsync():
	#
	
	gevent.spawn(getAoi)#因为这是在hub协程,不能有block的逻辑,但是getAoi发现有block的逻辑

# def wakeupAoiAsync():#被另一个线程调用
# 	global asyncWatcher
# 	if asyncWatcher:
# 		asyncWatcher.send()

def initAoiAsyncWatch():
	global asyncWatcher
	asyncWatcher = gevent.get_hub().loop.async()

	#todo 调用c接口,asyncWatcher设到底层去

	asyncWatcher.start(onAoiAsync)
	gevent.spawn_later(4,kill)


initAoiAsyncWatch()
gevent.get_hub().join()
print 'end alllllllllllll'


