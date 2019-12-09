using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Mail;
using System.Threading;
using System.Windows.Forms;

namespace RWHeartbeat
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();

            StopHeartbeat();

            LoadSMTPKey();
        }

        #region UI events

        private void btnStart_Click(object sender, EventArgs e)
        {
            StartHeartbeat();
        }

        private void btnStop_Click(object sender, EventArgs e)
        {
            StopHeartbeat();
        }

        private void btnRestart_Click(object sender, EventArgs e)
        {
            RestartProcess();
        }

        private void txtTimerInterval_TextChanged(object sender, EventArgs e)
        {
            StopHeartbeat();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            var url = txtRWurl.Text;
            var result = Ping(url);

            if (result)
            {
                txtLastHeartbeat.Text = DateTime.Now.ToString();
            }
            else
            {
                //email people that server failed heartbeat
                var message = "RW server failed heartbeat - restarting";
                SendEmail(message);

                timer1.Stop();

                RestartProcess();

                Thread.Sleep(10000); //wait 10 sec for server to come back up

                result = Ping(url);

                if (!result)
                {
                    //email people that server didn't work on restart
                    message = "!!!RW server heartbeat failed after restart!!!";
                    SendEmail(message);

                    StopHeartbeat();
                }
                else
                {
                    timer1.Start();
                }
            }
        }

        private void btnHeartbeat_Click(object sender, EventArgs e)
        {
            var url = txtRWurl.Text;
            var result = Ping(url);

            txtLastHeartbeat.Text = result
                ? DateTime.Now.ToString()
                : "failed heartbeat";
        }

        private void btnTestEmail_Click(object sender, EventArgs e)
        {
            SendEmail("RW Test Email");
        }

        #endregion

        private void StartHeartbeat()
        {
            if (!Int32.TryParse(txtTimerInterval.Text, out var interval)) return;

            timer1_Tick(null, null);

            btnStart.Enabled = false;
            btnStop.Enabled = true;
            timer1.Interval = interval * 1000;
            timer1.Start();
        }

        private void StopHeartbeat()
        {
            btnStart.Enabled = true;
            btnStop.Enabled = false;
            timer1.Stop();
        }

        private bool Ping(string url)
        {
            try
            {
                var request = (HttpWebRequest)WebRequest.Create(url);
                request.Timeout = 3000;
                request.AllowAutoRedirect = false;
                request.Method = "POST";

                using (request.GetResponse())
                {
                    return true;
                }
            }
            catch
            {
                return false;
            }
        }

        private void RestartProcess()
        {
            var path = txtRWProcess.Text;

            var processName = Path.GetFileNameWithoutExtension(path);

            var processes = Process.GetProcessesByName(processName);

            if (processes.Length > 0)
            {
                var process = processes[0];
                process?.Kill();

                Thread.Sleep(3000);
            }

            try
            {
                Process.Start(path);
            }
            catch (Exception)
            {
                //email people that error occured
                var message = "!!!RW server unable to restart!!!";
                SendEmail(message);

                StopHeartbeat();
            }
        }

        private void SendEmail(string subject)
        {
            var smtpHostUrl = "smtp.sendgrid.net";
            var username = "apikey";
            var password = txtSMTPKey.Text;

            SmtpClient smtpClient = new SmtpClient(smtpHostUrl, 587)
            {
                EnableSsl = true,
                Timeout = 10000,
                DeliveryMethod = SmtpDeliveryMethod.Network,
                UseDefaultCredentials = false,
                Credentials = new NetworkCredential(username, password)
            };

            MailMessage mailMsg = new MailMessage
            {
                From = new MailAddress("noreply@rwserver.com"),
                Subject = subject,
                Body = ""
            };
            mailMsg.To.Add(txtEmailRecipients.Text);

            smtpClient.Send(mailMsg);
        }

        private void LoadSMTPKey()
        {
            try
            {
                string[] lines = File.ReadAllLines(@"SMTPKey.txt");

                txtSMTPKey.Text = lines[0];

                for (int i = 1; i < lines.Length; i++)
                {
                    txtEmailRecipients.Text += lines[i] + "\r\n";
                }
            }
            catch
            {
            }
        }
    }
}
