/*
Zepto(document).on('ajaxBeforeSend',function(e,xhr,options){
    console.log('here is ajaxBeforSend');
});

Zepto(document).on('ajaxSuccess', function(e,xhr,options) {
   console.log('here is ajaxSUCCESS');
});
*/

var CLICKEVENT = 'tap';
var ISMOBILE = 1;
var username="";
var password="";

/*
function testAjax() {
    Zepto.ajax({type:'get',url:'/ajax', data:{name:'黑马'}, success:function(data){
        console.log("ajax success:"+ data);
    }, error: function(xhr, type){
            alert('Ajax error!');
            console.log("xhr="+xhr);
            console.log("type="+type);
        }
    });
}
*/
/*
function loadGames() {
    Zepto.ajax({type:'get',url:'/data', data:{cmd:'allString'}, success:function(data){
        console.log("ajax success:"+ data);
        Zepto(document.body).append(data);

    }, error: function(xhr, type){
        alert('Ajax error!');
        console.log("xhr="+xhr);
        console.log("type="+type);
    }
    });
}
*/

/*
function loadGamesJson() {
    Zepto.ajax({type:'get',url:'/data', data:{cmd:'allJson'}, dataType:'json',success:function(data){
        jsonStr = JSON.stringify(data);
        console.log("ajax success:"+ jsonStr);
        Zepto(document.body).append(jsonStr);

    }, error: function(xhr, type){
        alert('Ajax error!');
        console.log("xhr="+xhr);
        console.log("type="+type);
    }
    });
}
*/

var alreadyLoadGameCnt=0;
var perLoadCnt = 8;

//fromId：已经加载的资源个数
//perLoadCnt：每一次页面加载的个数，这里为8个
//kind：类型，最新文件、共享文件
function loadGamesBar(fromId, cnt, kind) {
	
	//Request URL:http://192.168.31.109/data?cmd=newFile&fromId=0&count=8&user=mike
    Zepto.ajax(
		{type:'get',url:'/data', data:{cmd: kind, fromId:fromId, count:cnt, user:getCookieValue("username")}, dataType:'json',
		
		//function(data) 为请求成功后的回调函数
		//data 为服务器返回的数据
		success:function(data)
		{
			//eval() 函数可计算某个字符串，并执行其中的的 JavaScript 代码。
			var games = eval(data.games);
			for (var i = 0; i < games.length; i++) 
			{
				addOneGameBar(games[i].id, games[i].title_m, games[i].url, games[i].picurl_m, games[i].pv, games[i].descrip, games[i].hot, games[i].title_s); //动态添加资料图标
				alreadyLoadGameCnt++;
			}
			myScroll.refresh();//刷新列表
		}, 
		
		error: function(xhr, type)
		{
			//alert('Ajax error!');
			console.log("xhr="+xhr);
			console.log("type="+type);
		}
    }
	); //end ajax
}

//fromId：已经加载的资源个数
//perLoadCnt：每一次页面加载的个数，这里为8个
//kind：类型，最新文件、共享文件
function loadGamesBarBykind(fromId, cnt, kind, type) {
    Zepto.ajax({type:'get',url:'/data', data:{cmd: kind, fromId:fromId, count:cnt, kind:type}, dataType:'json',success:function(data){
        var games = eval(data.games);
        for (var i = 0; i < games.length; i++) {
            addOneGameBar(games[i].id, games[i].title_m, games[i].url, games[i].picurl_m, games[i].pv, games[i].descrip, games[i].hot,games[i].title_s);
            alreadyLoadGameCnt++;
        }
        myScroll.refresh();
    }, error: function(xhr, type){
        //alert('Ajax error!');
        console.log("xhr="+xhr);
        console.log("type="+type);
    }
    });
}

//下载按钮处理函数
function clickGame(id,name,url) {
    Zepto.ajax({type:'get', url:'/data', data:{cmd: 'increase', fileId:id}, dataType:'json', success:function(data) {
        //console.log("post success!!!" + data.result);
    }, error: function(xhr,type){
        console.log("xhr="+xhr);
        console.log("type="+type);
    }});

	//切换页面，url为存放文件存放在fastdfs分布式存储服务器的url地址
    location.href=url;
}

//分享文件处理函数
function shareFile(id, username) {
    Zepto.ajax({type:'get', url:'/data', data:{cmd: 'shared', fileId:id, user:username}, dataType:'json', success:function(data) {
        //console.log("post success!!!" + data.result);
    }, error: function(xhr,type){
        console.log("xhr="+xhr);
        console.log("type="+type);
    }});

	//切换根目录页面
    location.href="/";
}

//作为游戏图片的id索引值
var game_logo_id = 0;
//动态添加资源图标
function addOneGameBar(id, name, url, picurl, pv, desc, hot, user) {
    var liObj = Zepto("<li>");
    var barObj = Zepto("<div>").attr("class", "game_opt");
	//下载，分享按钮
    var aObj = Zepto("<a>").html("下载");
    var bObj = Zepto("<a>").html("分享");
    //var canvasObj = Zepto("<canvas>").attr("id", "game_logo_" + game_logo_id).attr("width", 60).attr("height", 60);
    var gameLogoObj = Zepto("<img>").attr("id", "game_logo_" + game_logo_id).attr("width", 60).attr("height", 60).attr("class", "game_logo_img");
    var infoObj = Zepto("<div>").attr("class", "game_info");

    var gameNameObj = Zepto("<span>").attr("class", "game_name").html(name); //文件名字

    var hotObj = Zepto("<div>").attr("class", "game_hot");
    var starObj = Zepto("<span>").attr("class", "star_icon");
    var pvObj = Zepto("<i>").html("下载次数: "+pv);		//下载次数
    var descObj = Zepto("<p>").html(desc+" ["+user+"]");//文件创建时间、用户信息


    if (hot == 1) {//如果文件已经共享了
        var titleObj = Zepto("<span>").attr("class", "img_title_new");
        barObj.append(titleObj);
        titleObj.html("共享");

        bObj.css("color", "#ffffff");
        bObj.css("background-color", "#eeb4b4");
        bObj.css("border-color", "#eeb4b4");
    }


	//追加
    hotObj.append(starObj);
    hotObj.append(pvObj);

    infoObj.append(gameNameObj);
    infoObj.append(hotObj);
    infoObj.append(descObj);

    //barObj.append(canvasObj);
    barObj.append(gameLogoObj);
    barObj.append(infoObj);
    barObj.append(aObj);
    barObj.append(bObj);
    liObj.append(barObj).appendTo("#thelist");
    gameLogoObj.attr("src", picurl);
    //createCanvasFromUrl(game_logo_id,picurl, 0, 0);
/*    preImage(picurl, function() {
        gameLogoObj.attr("src", picurl);
    });*/
    game_logo_id++;


    /*
    if (ISMOBILE == 1) {
        barObj.on(CLICKEVENT, function(e){
            clickGame(id, name, url);
        });
    }
    */
	//点击下载按钮
    aObj.on(CLICKEVENT, function(e) { //事件响应
        clickGame(id, name, url);
    });
    aObj.on(CLICKEVENT, function(e){
        aObj.css("color", "#ffffff");
        aObj.css("background-color", "#0c4bba");
        aObj.css("border-color", "#0c4bba");
    });
    aObj.on('longTap', function(e){
        aObj.css("color", "#ffffff");
        aObj.css("background-color", "#0c4bba");
        aObj.css("border-color", "#0c4bba");
    });



    if (hot != 1) {
		//点击分享按钮
        bObj.on(CLICKEVENT, function(e) {
            shareFile(id, getCookieValue("username"))
        });

        bObj.css("background-color", "#ff6a6a");
        bObj.css("border-color", "#ff6a6a");

        bObj.on(CLICKEVENT, function(e){
            bObj.css("color", "#ffffff");
            bObj.css("background-color", "#eeb4b4");
            bObj.css("border-color", "#eeb4b4");
        });
        bObj.on('longTap', function(e){
            bObj.css("color", "#ffffff");
            bObj.css("background-color", "#eeb4b4");
            bObj.css("border-color", "#eeb4b4");
        });

    }

}

function createCanvasFromUrl(id, picurl, x, y) {
    var canvas = document.getElementById("game_logo_"+id);

    if (canvas.getContext){
        var context = canvas.getContext('2d');
        preImage(picurl,function(){
            context.drawImage(this,x,y,60,60);
        });
    }
}

//由于image加载是异步的，应该在image加载完毕时，调用drawImage方法绘制图像；可利用方法：
function preImage(url,callback){
    var img = new Image(); //创建一个Image对象，实现图片的预下载
    img.src = url;

    if (img.complete) { // 如果图片已经存在于浏览器缓存，直接调用回调函数
        callback.call(img);
        return; // 直接返回，不用再处理onload事件
    }

    img.onload = function () { //图片下载完毕时异步调用callback函数。
        callback.call(img);//将回调函数的this替换为Image对象
    };
}



/*
 *  iscroll  function
 *
 *
  */


var myScroll,
    pullDownEl, pullDownOffset,
    pullUpEl, pullUpOffset,
    generatedCount = 0;

function pullDownAction () {

    setTimeout(function () {	// <-- Simulate network congestion, remove setTimeout from production!
        /*
        var el, li, i;
        el = document.getElementById('thelist');

        for (i=0; i<3; i++) {
            li = document.createElement('li');
            li.innerText = 'Generated row ' + (++generatedCount);
            el.insertBefore(li, el.childNodes[0]);
        }
        */
        myScroll.refresh();		// Remember to refresh when contents are loaded (ie: on ajax completion)
    }, 500);	// <-- Simulate network congestion, remove setTimeout from production!

}

//上拉动作响应
function pullUpAction () {
	//1000毫秒后，才更新
    setTimeout(function () {	// <-- Simulate network congestion, remove setTimeout from production!

        /*
        var el, li, i;
        el = document.getElementById('thelist');

        for (i=0; i<3; i++) {
            li = document.createElement('li');
            li.innerText = 'Generated row ' + (++generatedCount);
            el.appendChild(li, el.childNodes[0]);
        }
        */
        if (menu_game_new_selected == 1) {
            loadGamesBar(alreadyLoadGameCnt,perLoadCnt,'newFile'); //最新文件
        } else if (menu_game_hot_selected == 1) {
            loadGamesBar(alreadyLoadGameCnt,perLoadCnt,'hotGame');	//共享文件
        } else if (menu_kind_selected == 1 || menu_kind_selected_type != '0') {
			//最近文件，共享文件，处理函数
			//loadGamesBarBykind()和loadGamesBar()区别，loadGamesBar()中的alreadyLoadGameCnt每次都是0
			//loadGamesBarBykind()中alreadyLoadGameCnt的值为上一次的记录
			//至于内部实现的功能，差不多
			//alreadyLoadGameCnt：已经加载的资源个数
			//perLoadCnt：每一次页面加载的个数，这里为8个
			//kind：类型，最新文件、共享文件
            loadGamesBarBykind(alreadyLoadGameCnt,perLoadCnt,'kind', menu_kind_selected_type);
        }

    }, 1000);	// <-- Simulate network congestion, remove setTimeout from production!
}

function loaded() {

    pullDownEl = document.getElementById('pullDown');
    pullDownOffset = pullDownEl.offsetHeight;
    pullUpEl = document.getElementById('pullUp');
    pullUpOffset = pullUpEl.offsetHeight;

    myScroll = new iScroll('wrapper', {
        useTransition: true,
        topOffset: pullDownOffset,
        onRefresh: function () {
            if (pullDownEl.className.match('loading')) {
                pullDownEl.className = '';
                //pullDownEl.querySelector('.pullDownLabel').innerHTML = 'Pull down to refresh...';
            } else if (pullUpEl.className.match('loading')) {
                pullUpEl.className = '';
                pullUpEl.querySelector('.pullUpLabel').innerHTML = '轻轻上拉 更多文件...';
            }
        },
        onScrollMove: function () {
            hideMenu();
            if (this.y > 5 && !pullDownEl.className.match('flip')) {
                pullDownEl.className = 'flip';
                //pullDownEl.querySelector('.pullDownLabel').innerHTML = 'Release to refresh...';
                this.minScrollY = 0;
            } else if (this.y < 5 && pullDownEl.className.match('flip')) {
                pullDownEl.className = '';
                //pullDownEl.querySelector('.pullDownLabel').innerHTML = 'Pull down to refresh...';
                this.minScrollY = -pullDownOffset;
            } else if (this.y < (this.maxScrollY - 5) && !pullUpEl.className.match('flip')) {
                pullUpEl.className = 'flip';
                pullUpEl.querySelector('.pullUpLabel').innerHTML = '刷新中...';
                this.maxScrollY = this.maxScrollY;
            } else if (this.y > (this.maxScrollY + 5) && pullUpEl.className.match('flip')) {
                pullUpEl.className = '';
                pullUpEl.querySelector('.pullUpLabel').innerHTML = '轻轻上拉 更多文件...';
                this.maxScrollY = pullUpOffset;
            }
        },
        onScrollEnd: function () { //如果有上拉
            if (pullDownEl.className.match('flip')) {
                pullDownEl.className = 'loading';
                //pullDownEl.querySelector('.pullDownLabel').innerHTML = '上面没有游戏哦...';
                pullDownAction();	// Execute custom function (ajax call?)
            } else if (pullUpEl.className.match('flip')) {
                pullUpEl.className = 'loading';
                pullUpEl.querySelector('.pullUpLabel').innerHTML = '更新中...';
                pullUpAction();	// Execute custom function (ajax call?) //指向上拉动作
            }
        }
    });

    setTimeout(function () { document.getElementById('wrapper').style.left = '0'; }, 800);
    selected_action('newFile');//处理最新文件的的动作
}

function hideMenu() {
    Zepto(".second_menu_show").attr("class", "second_menu_hide");
    menu_kind_selected = 0;
}

document.addEventListener('touchmove', function (e) { e.preventDefault(); }, false);

//setTimeout 指定的毫秒数后调用函数loaded()， 不会循环调用
document.addEventListener('DOMContentLoaded', function () { setTimeout(loaded, 200); }, false);

var menu_game_new_selected = 0;
var menu_game_hot_selected = 0;
var menu_kind_selected = 0;
var menu_group_selected = 0;
var menu_kind_selected_type = '0';

//选择动作
function selected_action(kind) {

    if (kind == 'newFile') { //最新文件
        //未被点中
        if (menu_game_new_selected == 0) {
            menu_game_new_selected = 1;
            menu_game_hot_selected = 0;
            menu_kind_selected = 0;

            //设置其他两个按钮未选中
            Zepto(".hot_icon img").attr("src", "static/img/icon_2@2x.png");
            Zepto(".kind_icon img").attr("src", "static/img/icon_3@2x.png");

            //使本按钮选中
            Zepto(".new_icon img").attr("src", "static/img/icon_11@2x.png")

            reloadGames('newFile'); //响应动作

        }
        //已经点中
        else {
            hideMenu();
            return;
        }
    } else if( kind == 'shareFile') { //共享文件
        if (menu_kind_selected == 0) {
            menu_kind_selected = 1;
            menu_game_new_selected=0;
            menu_game_hot_selected=0;
            //设置其他两个按钮未选中
            Zepto(".new_icon img").attr("src", "static/img/icon_1@2x.png");
            Zepto(".hot_icon img").attr("src", "static/img/icon_2@2x.png");
            Zepto(".kind_icon img").attr("src", "static/img/icon_31@2x.png");

            reloadGames('shareFile');//响应动作
        }
        else {
            menu_kind_selected = 0;
            hideMenu();
        }
    } else if( kind == 'hotGame') {//文件上传
        if (menu_game_hot_selected == 0) {
            menu_game_hot_selected = 1;
            menu_game_new_selected = 0;
            menu_kind_selected = 0;

            //设置其他两个按钮未选中
            Zepto(".new_icon img").attr("src", "static/img/icon_1@2x.png");
            Zepto(".kind_icon img").attr("src", "static/img/icon_3@2x.png");
            Zepto(".hot_icon img").attr("src", "static/img/icon_21@2x.png");

            //reloadGames('hotGame');
            location.href = "/upload.html"; //切换页面

        }
        else {
            return;
        }


    } else if (kind == 'group') {//登陆
        if (menu_group_selected == 0) {
            menu_group_selected = 1;
            menu_game_new_selected = 0;
            menu_kind_selected = 0;
            menu_game_hot_selected = 0;

            //设置其他三个按钮未选中
            Zepto(".new_icon img").attr("src", "static/img/icon_1@2x.png");
            Zepto(".kind_icon img").attr("src", "static/img/icon_3@2x.png");
            Zepto(".hot_icon img").attr("src", "static/img/icon_2@2x.png");
            Zepto(".group_icon img").attr("src", "static/img/icon_41@2x.png");

            location.href = "/login.html";	//切换页面
        }


    } else if (kind == 'kind') {//这里没有用上
        if (menu_kind_selected == 0) {
            menu_kind_selected = 1;
            menu_game_new_selected=0;
            menu_game_hot_selected=0;
            //设置其他两个按钮未选中
            Zepto(".new_icon img").attr("src", "static/img/icon_1@2x.png");
            Zepto(".hot_icon img").attr("src", "static/img/icon_2@2x.png");
            Zepto(".kind_icon img").attr("src", "static/img/icon_31@2x.png");


            Zepto("#second_menu").each(function(index){
                Zepto(this).attr('class','second_menu_show');
            });

            Zepto("#menu_kind_0 i").html("("+gameCntAll+")");
            Zepto("#menu_kind_1 i").html("("+gameCnt_1+")");
            Zepto("#menu_kind_2 i").html("("+gameCnt_2+")");
            Zepto("#menu_kind_3 i").html("("+gameCnt_3+")");
            Zepto("#menu_kind_4 i").html("("+gameCnt_4+")");
            Zepto("#menu_kind_5 i").html("("+gameCnt_5+")");
            Zepto("#menu_kind_6 i").html("("+gameCnt_6+")");


            return;
        }
        else {
            menu_kind_selected = 0;
            hideMenu();
        }
    } else {//其它
        menu_kind_selected_type = kind.split('_')[1];

        reloadGamesKind('kind', menu_kind_selected_type);
    }

}



function reloadGames(kind) {
    //隐藏二级菜单
    hideMenu();
    //清楚iScroll元素
    Zepto("#thelist li:not(:first-child)").remove();
    //游戏计数清零
    alreadyLoadGameCnt = 0;
	
	
    //最近文件，共享文件，处理函数
	//alreadyLoadGameCnt：已经加载的资源个数，从0开始
	//perLoadCnt：每一次页面加载的个数，这里为8个
	//kind：类型，最新文件、共享文件
    loadGamesBar(alreadyLoadGameCnt, perLoadCnt, kind);


    myScroll.refresh();
}
function reloadGamesKind(kind,type) {
    //隐藏二级菜单
    hideMenu();
    //清楚iScroll元素
    Zepto("#thelist li:not(:first-child)").remove();
    //游戏计数清零
    alreadyLoadGameCnt = 0;
    //重新加载一类游戏
    loadGamesBarBykind(alreadyLoadGameCnt, perLoadCnt, kind, type);


    myScroll.refresh();
}


//所有游戏个数
var gameCntAll = 0;
//体育竞技游戏个数
var gameCnt_1 = 0;
//休闲益智游戏个数
var gameCnt_2 = 0;
//角色扮演游戏个数
var gameCnt_3 = 0;
//飞行射击游戏个数
var gameCnt_4 = 0;
//棋牌娱乐游戏个数
var gameCnt_5 = 0;
//女孩专题游戏个数
var gameCnt_6 = 0;

function selecteGameCnt() {
    Zepto.ajax({type:'get',url:'/data', data:{cmd: 'selectCnt', kind: 'all'}, dataType:'json',success:function(data){
        var allData = eval(data);
        gameCntAll = allData['kind_0'];
        gameCnt_1 = allData['kind_1'];
        gameCnt_2 = allData['kind_2'];
        gameCnt_3 = allData['kind_3'];
        gameCnt_4 = allData['kind_4'];
        gameCnt_5 = allData['kind_5'];
        gameCnt_6 = allData['kind_6'];
    }, error: function(xhr, type){
        //alert('Ajax error!');
        console.log("xhr="+xhr);
        console.log("type="+type);
    }});
}



var layerIsShow = 0;
function showOutLayer() {
    if (layerIsShow == 0) {
        Zepto("#out_layer").show();
        Zepto("#point").show();
        layerIsShow = 1;
    }
    else {
        Zepto("#out_layer").hide();
        Zepto("#point").hide();
        layerIsShow = 0;
    }
}

function checkPC() {
    //判断浏览器客户端
    var sUserAgent = navigator.userAgent.toLowerCase();
    var bIsIpad = sUserAgent.match(/ipad/i) == "ipad";
    var bIsIphoneOs = sUserAgent.match(/iphone os/i) == "iphone os";
    var bIsMidp = sUserAgent.match(/midp/i) == "midp";
    var bIsUc7 = sUserAgent.match(/rv:1.2.3.4/i) == "rv:1.2.3.4";
    var bIsUc = sUserAgent.match(/ucweb/i) == "ucweb";
    var bIsAndroid = sUserAgent.match(/android/i) == "android";
    var bIsCE = sUserAgent.match(/windows ce/i) == "windows ce";
    var bIsWM = sUserAgent.match(/windows mobile/i) == "windows mobile";

    if (bIsIpad || bIsIphoneOs || bIsMidp || bIsUc7 || bIsUc || bIsAndroid || bIsCE || bIsWM) {//如果是上述设备就会以手机域名打开
        ISMOBILE = 1;
        CLICKEVENT = 'tap';
    }else{//否则就是电脑域名打开
        ISMOBILE = 0;
        CLICKEVENT = 'click';
    }
}

function bindEvent() {
    Zepto("#menu_game_new").on(CLICKEVENT, function(e) {//点击"最新文件"
        selected_action('newFile');
    });

    Zepto("#menu_game_hot").on(CLICKEVENT, function(e) {//点击"文件上传"
        selected_action('hotGame');
    });

    Zepto("#menu_group").on(CLICKEVENT, function(e){//点击"登陆"
        selected_action('group');
    });

    /*
    Zepto("#menu_kind").on(CLICKEVENT, function(e) {
        selected_action('kind');
    });
    */
    Zepto("#menu_kind").on(CLICKEVENT, function(e) {//点击"共享文件"
        selected_action('shareFile');
    });

    Zepto("#menu_kind_0").on(CLICKEVENT, function(e){//全部文件, 和点击"最近文件"效果一样
        selected_action('newFile');
    });
    Zepto("#menu_kind_1").on(CLICKEVENT, function(e){
        selected_action('kind_1');
    });
    Zepto("#menu_kind_2").on(CLICKEVENT, function(e){
        selected_action('kind_2');
    });
    Zepto("#menu_kind_3").on(CLICKEVENT, function(e){
        selected_action('kind_3');
    });
    Zepto("#menu_kind_4").on(CLICKEVENT, function(e){
        selected_action('kind_4');
    });
    Zepto("#menu_kind_5").on(CLICKEVENT, function(e){
        selected_action('kind_5');
    });
    Zepto("#menu_kind_6").on(CLICKEVENT, function(e){
        selected_action('kind_6');
    });

    Zepto("#attention").on(CLICKEVENT, function(e) { //"关注"超链接
        location.href="http://mp.weixin.qq.com/s?__biz=MzI0NjM3NjI1NQ==&mid=2247484325&idx=2&sn=d5ab0cf8348275265eb285bc7c1934fa&chksm=e94171f5de36f8e3415e3b8532444dae2e93c6160e0f2957c9a96e97218390cb5914735163d4#rd"
    });

    Zepto('#out_layer').on(CLICKEVENT, function(e){ //布局
        showOutLayer();
    });

    Zepto('#attention_please').on(CLICKEVENT, function(e){
        showOutLayer();
    });
}

//window.onload = function(){}加载页面后就要立即执行，而function(){}需要调用才能执行
window.onload = function() {
    //统计各种类文件个数
    //selecteGameCnt();

    checkPC(); //判断浏览器客户端，为了兼容浏览器

    bindEvent(); //绑定事件，点击按钮后的响应函数

	/**获取cookie的值，根据cookie的键获取值**/  
    username = getCookieValue("username");
    password = getCookieValue("passwd");

    if (username == "") {//如果用户名为空，说明用户没有登录
        alert("请先登录");
        location.href="/login.html"; //切换登陆界面
    }


    if (ISMOBILE == 0) {//如果是手机，换样式
        Zepto("#wrapper").css("width", "33%");
        Zepto("#wrapper").css("margin-left", "33%");
        Zepto("#header").css("width", "33%");
        Zepto("#header").css("margin-left", "33%");
        Zepto("#footer").css("width", "33%");
        Zepto("#footer").css("margin-left", "33%");

        Zepto("#attention").hide();
        Zepto("#attention_please").hide();
        Zepto("#weixin_qrcode").show();
        Zepto("#qrcode_desc").show();
    }
};

