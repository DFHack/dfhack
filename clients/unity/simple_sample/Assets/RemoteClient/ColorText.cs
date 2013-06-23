using System;
using UnityEngine;
using AT.MIN;
using dfproto;

namespace  DFHack
{
    public enum color_value
    {
        COLOR_RESET = -1,
        COLOR_BLACK = 0,
        COLOR_BLUE,
        COLOR_GREEN,
        COLOR_CYAN,
        COLOR_RED,
        COLOR_MAGENTA,
        COLOR_BROWN,
        COLOR_GREY,
        COLOR_DARKGREY,
        COLOR_LIGHTBLUE,
        COLOR_LIGHTGREEN,
        COLOR_LIGHTCYAN,
        COLOR_LIGHTRED,
        COLOR_LIGHTMAGENTA,
        COLOR_YELLOW,
        COLOR_WHITE,
        COLOR_MAX = COLOR_WHITE
    };

    public class color_ostream
    {
        string buffer;
        public void printerr(string Format, params object[] Parameters)
        {
            Debug.LogError(Tools.sprintf(Format, Parameters));
        }
        public void print(string Format, params object[] Parameters)
        {
            Debug.Log(Tools.sprintf(Format, Parameters));
        }
        public void begin_batch()
        {
            buffer = "";
        }
        public void end_batch()
        {
            Debug.Log(buffer);
            buffer = null;
        }

        public void add_text(color_value color, string text)
        {
            //Debug.Log(text);
            buffer += text;
        }

    }
    public class buffered_color_ostream : color_ostream
    {
    //protected:
    public new void add_text(color_value color, string text)
    {
        if (text.Length == 0)
            return;

        if (buffer.Length == 0)
        {
            buffer = text;
        }
        else
        {
            buffer += text;
        }
    }



    //    buffered_color_ostream() {}
    //    ~buffered_color_ostream() {}

    //    const std::list<fragment_type> &fragments() { return buffer; }

    protected string buffer;
    }
    public class color_ostream_proxy : buffered_color_ostream
    {
        protected color_ostream target;

        //virtual void flush_proxy();

        public color_ostream_proxy(color_ostream targetIn)
        {
            target = targetIn;
        }

        public virtual color_ostream proxy_target() { return target; }

        public void decode(dfproto.CoreTextNotification data)
        {
            int cnt = data.fragments.Count;

            if (cnt > 0)
            {
                target.begin_batch();

                for (int i = 0; i < cnt; i++)
                {
                    var frag = data.fragments[i];

                    //color_value color = frag.has_color() ? color_value(frag.color()) : COLOR_RESET;
                    target.add_text(color_value.COLOR_RESET, frag.text);
                    //target.printerr(data.fragments[i].text);
                }

                target.end_batch();
            }
        }
    }
}