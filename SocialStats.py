import sqlite3
import paho.mqtt.client as mqtt
import aiohttp
import asyncio
import re
import json

class SocialStats:
    def __init__(self):
        self.twitchSession = aiohttp.ClientSession(headers={'Client-ID' : "Enter Twitch API ID Here"})
        self.instagramSession = aiohttp.ClientSession()
        self.twitterSession = aiohttp.ClientSession()
        
        self.mqttClient = mqtt.Client()
        self.mqttClient.connect("broker.mqtt-dashboard.com")
        self.mqttClient.loop_start()
        
        self.file = "SocialDB.sqlite"
        self.connection = sqlite3.connect(self.file)
        self.sqlDB = self.connection.cursor()
        
    async def TwitchFollowers(self, user):
        try:
            async with self.twitchSession.get('https://api.twitch.tv/helix/users/follows?to_id=' + user + '&first=1')  as resp:
                if resp.status == 200:
                    twitchJson = await resp.json()
                    return twitchJson["total"]
        except Exception as e:
            print(type(e).__name__ + str(e))
            
    
    async def InstagramFollowers(self, user):
        try:
            async with self.instagramSession.get('https://www.instagram.com/' + user + '/')  as resp:
                if resp.status == 200:
                    result = re.search('<meta property="og:description" content="(.+?) Followers', await resp.text())
                    return int(result.group(1))
                else:
                    print(resp.status)
        except Exception as e:
            print(type(e).__name__ + str(e))
    
    async def TwitterFollowers(self, user):
        try:
            async with self.twitterSession.get('http://cdn.syndication.twimg.com/widgets/followbutton/info.json?screen_names=' + user)  as resp:
                if resp.status == 200:
                    twitterJson = await resp.json()
                    return twitterJson[0]["followers_count"]
        except Exception as e:
            print(type(e).__name__ + str(e))
            
    def ParseImage(self, platform):
        self.sqlDB.execute('SELECT * FROM SocialInfo WHERE Platform = "{}"'.format(platform))
        image = self.sqlDB.fetchone()[1]
        return [int(x) for x in image.split(',')]
            
    def BuildJson(self, twitchCount, instaCount, twitterCount):
        data = {}
        data['data'] = []
        count = 0
        if twitchCount >= 0:
            count = count + 1
            stats = {'image' : self.ParseImage("Twitch"), 'value' : twitchCount}
            data['data'].append(stats)
        if instaCount >= 0:
            count = count + 1
            stats = {'image' : self.ParseImage("Instagram"), 'value' : instaCount}
            data['data'].append(stats)
        if twitterCount >= 0:
            count = count + 1
            stats = {'image' : self.ParseImage("Twitter"), 'value' : twitterCount}
            data['data'].append(stats)
            
        data['count'] = count
        return json.dumps(data)
            
    
    async def Loop(self):
        while(True):
            self.sqlDB.execute("SELECT * FROM Users")
            for user in self.sqlDB.fetchall():
                twitchCount = "-1"
                instaCount = "-1"
                twitterCount = "-1"
                
                if user[1]:
                    twitchCount = await self.TwitchFollowers(user[1])
                if user[3]:
                    instaCount = await self.InstagramFollowers(user[3])   
                if user[5]:
                    twitterCount = await self.TwitterFollowers(user[5]) 
                    
                if twitchCount != user[2] or instaCount != user[4] or twitterCount != user[6]:
                    self.sqlDB.execute('UPDATE Users SET TwitchCount = {}, InstaCount = {}, TwitterCount = {} WHERE Username = "{}"'.format(twitchCount, instaCount, twitterCount, user[0]))
                    self.connection.commit()
                    
                    self.mqttClient.publish(user[0], self.BuildJson(twitchCount, instaCount, twitterCount), 0, True)
                    
            await asyncio.sleep(60)
        
    
loop = asyncio.get_event_loop()
social = SocialStats()
loop.run_until_complete(social.Loop())