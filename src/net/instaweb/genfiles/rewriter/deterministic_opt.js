(function(){var a="length";var b=Date,c=0,d=0,e=0.462,f=1204251968254;Math.random=function(){c++;25<c&&(e+=0.1,c=1);return e%1};Date=function(){if(this instanceof Date)switch(d++,25<d&&(f+=50,d=1),arguments[a]){case 0:return new b(f);case 1:return new b(arguments[0]);default:return new b(arguments[0],arguments[1],3<=arguments[a]?arguments[2]:1,4<=arguments[a]?arguments[3]:0,5<=arguments[a]?arguments[4]:0,6<=arguments[a]?arguments[5]:0,7<=arguments[a]?arguments[6]:0)}return(new Date).toString()};Date.__proto__=b;b.now=function(){return(new Date).getTime()};})();
