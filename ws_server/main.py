from tornado.web import Application, RequestHandler
from tornado.ioloop import IOLoop
from tornado.websocket import WebSocketHandler
import json
import string
import random
import os
conn = None
GameCredentials={}
GameAdminMapper={}
GameJoinedUser={}
GameUserMapper={}

def key_generator(size=6, chars=string.ascii_lowercase):
    return ''.join(random.choice(chars) for _ in range(size))


class SocketHandler(WebSocketHandler):
    def check_origin(self, origin):
        return True

    def open(self):
        pass

    def on_message(self, message):
        print(message)
        try:
            data = json.loads(message)
            action=data['action']
            if action=="join_admin":
                payload=data['payload']
                username=payload['username']
                password=payload['password']
                key=payload['key']
                if username in GameCredentials:
                    if GameCredentials[username][0]==password and GameCredentials[username][1]==key:
                        GameAdminMapper[username]=self
                        GameUserMapper[self]=username
                        self.write_message(json.dumps({"action": "join_admin_response", "status": True, "message": "You Joined Room Sucessfully"}))
                    else:
                        self.write_message(json.dumps({"action": "join_admin_response", "status": False, "message": "Incorrect Room Credentials"}))
                else:
                    self.write_message(json.dumps({"action":"join_admin_response","status":False,"message":"Room Not Found"}))
            elif action=="join":
                payload = data['payload']
                username = payload['username']
                password = payload['password']
                if username in GameCredentials:
                    if GameCredentials[username][0] == password:

                        try:

                            admin=GameAdminMapper[username]
                        except KeyError:
                            self.write_message(json.dumps({"action": "join_response", "status": False, "message": "Admin Not Joinde Game"}))
                            return

                        try:

                            users=GameJoinedUser[username]
                            userNo=len(users)
                            id=userNo
                            users[userNo]=self
                            GameJoinedUser[username]=users
                        except KeyError:
                            users={}
                            id=0
                            users[0]=self
                            GameJoinedUser[username] = users
                        GameUserMapper[self]=username
                        self.write_message(json.dumps({"action": "join_response", "status": True, "id":id,"message": "Join Room Sucessfully"}))
                        admin.write_message(json.dumps({"action":"create_offer","id":id}))
                    else:
                        self.write_message(json.dumps({"action": "join_response", "status": False, "message": "Incorrect Room Credentials"}))
                else:
                    self.write_message(json.dumps({"action":"join_response","status":False,"message":"Room Not Found"}))
            elif action=="send_offer":
                payload=data['payload']
                username=payload['username']
                id=payload['id']
                sdp=payload["sdp"]
                try:
                    admin=GameAdminMapper[username]
                    if admin==self:
                        if id in GameJoinedUser[username]:
                            GameJoinedUser[username][id].write_message(json.dumps({"action":"accept_offer","payload":{"sdp":sdp}}))
                except Exception as e:
                    print(e)
            elif action=="send_answer":
                payload=data["payload"]
                id=payload["id"]
                sdp=payload["sdp"]
                username=payload["username"]
                try:
                    if GameJoinedUser[username][id]==self:
                        admin=GameAdminMapper[username]
                        admin.write_message(json.dumps({"action":"accept_answer","payload":{"id":id,"sdp":sdp}}))
                except:
                    pass
            elif action=="send_candidate":
                payload=data["payload"]
                type=payload["type"]
                username=payload["username"]
                id=payload["id"]
                candidate=payload["candidate"]
                if type=="admin":
                    if GameAdminMapper[username]==self:
                        user=GameJoinedUser[username][id]
                        user.write_message(json.dumps({"action":"accept_candidate","payload":{"candidate":candidate}}))
                else:
                    admin=GameAdminMapper[username]
                    admin.write_message(json.dumps({"action":"accept_candidate","payload":{"id":id,"candidate":candidate}}))
            elif action=="answer_set":
                payload=data["payload"]
                id=payload["id"]
                username = payload["username"]
                if GameAdminMapper[username] == self:
                    user=GameJoinedUser[username][id]
                    user.write_message(json.dumps({"action":"gather_candidate"}))
                    self.write_message(json.dumps({"action": "gather_candidate","id":id}))


        except Exception as e:
            print(e,":Error")
            try:
                self.write_message(json.dumps({"action":"error","message":"Invalid Operation {}".format(e)}))
            except:
                pass

    def on_close(self):
        try:
            username=GameUserMapper[self]
            completed=False
            try:
                if GameAdminMapper[username]==self:
                    del GameAdminMapper[username]
                    del GameJoinedUser[username]
                    del GameCredentials[username]
                    completed=True
            except:
                pass
            if not completed:
                users=GameJoinedUser[username]
                for k, v in GameJoinedUser[username].items():
                    if v==self:
                        del users[k]
                        break
                GameJoinedUser[username]=users
        except:
            pass


class HelloHandler(RequestHandler):
    def get(self):
        self.write("Hello World!")

class CreateGame(RequestHandler):
    def post(self):
        data=json.loads(self.request.body.decode())
        username=data['username']
        password=data['password']
        if username not in GameCredentials:
            key=key_generator(size=10)
            credential=[]
            credential.append(password)
            credential.append(key)
            GameCredentials[username]=credential
            self.write(json.dumps({"status":True,"message":"Room Created Sucessfully!","key":key}))
        else:
            self.write(json.dumps({"status":False,"message":"Username Already Exist!"}))

def make_app():
    urls = [("/", HelloHandler),("/create_game", CreateGame),("/ws", SocketHandler)]
    return Application(urls)


if __name__ == '__main__':
    port = int(os.getenv('PORT', 8000))
    app = make_app()
    app.listen(port)
    IOLoop.instance().start()