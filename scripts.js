function toggleDarkMode () {
  var element = document.body;
  element.classList.toggle("dark-mode");
}

function create_link (image, url) {
   var e = document.createElement ("a");
   e.href = url;
   var img = document.createElement ("img");
   img.src = image;
   e.appendChild (img);
   return e;
}

function make_sharelinks (page_title, page_link) {
   page_link = "http://www.lelanthran.com/simply_wordy/articles/" + page_link + "/main.html";
   var share_twitter = create_link ("../../images/twitter-light.svg",
                                    "https://twitter.com/intent/tweet?"
                                  + "text=" + page_title
                                  + "&url="  + page_link);
   var share_reddit = create_link ("../../images/reddit-light.svg",
                                    "https://www.reddit.com/submit?"
                                  + "title=" + page_title
                                  + "&url="  + page_link);
   var share_hnews = create_link ("../../images/hackernews-light.svg",
                                    "https://news.ycombinator.com/submitlink?"
                                  + "t=" + page_title
                                  + "&u="  + page_link);
   var share_fb = create_link ("../../images/facebook-light.svg",
                                    "https://www.facebook.com/sharer/sharer.php?"
                                  + "&u="  + page_link);


   document.getElementById ("share_links_div").appendChild (share_twitter);
   document.getElementById ("share_links_div").appendChild (document.createElement ("br"));
   document.getElementById ("share_links_div").appendChild (share_fb);
   document.getElementById ("share_links_div").appendChild (document.createElement ("br"));
   document.getElementById ("share_links_div").appendChild (share_reddit);
   document.getElementById ("share_links_div").appendChild (document.createElement ("br"));
   document.getElementById ("share_links_div").appendChild (share_hnews);
}

var g_timeout1_id = 0;
var g_timeout2_id = 0;
function show_sharelinks () {
   clearTimeout (g_timeout1_id);
   clearTimeout (g_timeout2_id);
   document.getElementById ("share_links_div").style.display = 'block';
   document.getElementById ("share_links_div").classList.remove ("share_links_fade_out");
   document.getElementById ("share_links_div").classList.add ("share_links_fade_in");
   g_timeout1_id = setTimeout (
      function () {
         document.getElementById ("share_links_div").classList.remove ("share_links_fade_in");
         document.getElementById ("share_links_div").classList.add ("share_links_fade_out");
         clearTimeout (g_timeout2_id);
         g_timeout2_id = setTimeout (
            function () {
               document.getElementById ("share_links_div").style.display = 'none';
            },
            500);
      },
      3000);
}

