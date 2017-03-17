using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;


namespace WindowsFormsApplication1
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            pictureBox5.Load("C:\\panorama\\output1.jpg");
            //File.Delete("C:\\Users\\Hrishikesh\\Desktop\\output.jpg");
            string wave = "no";
            string warp = "";
            string blend = "";

           // Process process = new Process();
           // process.StartInfo.FileName = "C:\\Users\\Hrishikesh\\Documents\\Visual Studio 2015\\Projects\\ConsoleApplication1\\x64\\Release\\PanoramaSynthesis.exe";
           // process.StartInfo.Arguments = "t1.jpg t2.jpg t3.jpg t4.jpg --wave_correct horiz --warp cylindrical --blend feather"; // Put your arguments here
           // process.Start();

            if (radioButton1.Checked == true)
            {
                blend = "feather";
            }
            else if (radioButton2.Checked == true)
            {
                blend = "multiband";
            }

            if (radioButton4.Checked == true)
            {
                wave = "horiz";
            }
            else if (radioButton3.Checked == true)
            {
                wave = "vert";
            }
            else if (radioButton7.Checked == true)
            {
                wave = "no";
            }

            if (radioButton6.Checked == true)
            {
                warp = "cylindrical";
            }
            else if (radioButton5.Checked == true)
            {
                warp = "plane";
            }

           
            Process cmd = new Process();
            cmd.StartInfo.FileName = @"C:\panorama\PanoramaSynthesis.exe";
            cmd.StartInfo.Arguments = @"" + pictureBox1.ImageLocation + " " + pictureBox2.ImageLocation + " " + pictureBox3.ImageLocation + " " + pictureBox4.ImageLocation + " " + "--wave_correct " + wave + " --warp " + warp + " --blend " + blend;
            cmd.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;

            cmd.Start();
            cmd.WaitForExit();
            pictureBox5.Load("C:\\panorama\\output.jpg");
            pictureBox5.SizeMode = PictureBoxSizeMode.StretchImage;
        }

        private void button2_Click(object sender, EventArgs e)
        {

            string Path="";

            OpenFileDialog fb1 = new OpenFileDialog();
            if (fb1.ShowDialog() == DialogResult.OK)
            {
                Path = fb1.FileName;
                pictureBox1.Load(Path);
                pictureBox1.SizeMode = PictureBoxSizeMode.StretchImage;
            }
        }



        private void button3_Click(object sender, EventArgs e)
        {
            string Path = "";

            OpenFileDialog fb1 = new OpenFileDialog();
            if (fb1.ShowDialog() == DialogResult.OK)
            {
                Path = fb1.FileName;
                pictureBox2.Load(Path);
                pictureBox2.SizeMode = PictureBoxSizeMode.StretchImage;

            }
                
        }

     

        private void button4_Click(object sender, EventArgs e)
        {
            string Path = "";

            OpenFileDialog fb1 = new OpenFileDialog();
            if (fb1.ShowDialog() == DialogResult.OK)
            {
                Path = fb1.FileName;
                pictureBox3.Load(Path);
                pictureBox3.SizeMode = PictureBoxSizeMode.StretchImage;
            }
                
        }

        private void button5_Click(object sender, EventArgs e)
        {
            string Path = "";

            OpenFileDialog fb1 = new OpenFileDialog();
            if (fb1.ShowDialog() == DialogResult.OK)
            {
                Path = fb1.FileName;
                pictureBox4.Load(Path);
                pictureBox4.SizeMode = PictureBoxSizeMode.StretchImage;

            }
                
        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {

        }

        private void groupBox3_Enter(object sender, EventArgs e)
        {

        }
    }
}
